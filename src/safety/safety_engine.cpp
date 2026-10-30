// ============================================================================
// Suraksha+ Watch OS — Safety Engine Implementation
// ============================================================================
#include "safety_engine.h"
#include "../sensors/sensor_manager.h"
#include "../drivers/vibration.h"
#include "../platform/power_manager.h"
#include "../config/app_config.h"
#include "../comms/ble_service.h"
#include "../ui/ui_manager.h"

SafetyEngine& SafetyEngine::getInstance() {
    static SafetyEngine instance;
    return instance;
}

void SafetyEngine::init() {
    Serial.println("[SAFE] Initializing safety engine...");

    _sosManager.init();
    _gestureDetector.init();
    _fallDetector.init();
    _hrAnomaly.init();
    _activityMode.init();

    _state = SafetyState::IDLE;
    Serial.println("[SAFE] ✅ Safety engine ready");
}

void SafetyEngine::update() {
    SensorManager& sensors = SensorManager::getInstance();

    // Update gesture detector with current SOS status (locks taps during SOS)
    _gestureDetector.setSOSActive(_state == SafetyState::SOS_ACTIVE || _state == SafetyState::SOS_COUNTDOWN);
    _gestureDetector.update();

    // Process gesture actions
    int tapAction = _gestureDetector.consumeAction();
    bool longPress = _gestureDetector.consumeLongPress();

    if (tapAction > 0 || longPress) {
        _processGestureAction(tapAction, longPress);
    }

    // Update fall detector with IMU data
    if (sensors.isIMUAvailable()) {
        IMUData imuData = sensors.getIMU().getData();
        _fallDetector.update(imuData.accMagnitude, imuData.motionScore);
    }

    // Update HR anomaly detector
    if (sensors.isHeartRateAvailable()) {
        HeartRateData hrData = sensors.getHeartRate().getData();
        _hrAnomaly.update(hrData.bpm, hrData.fingerPresent);
    }

    // Update activity mode timer
    _activityMode.update();

    // State machine
    switch (_state) {
        case SafetyState::IDLE:
            // Check for fall detection
            if (_fallDetector.isFallDetected() && !_activityMode.isActive()) {
                _handleFallDetection();
            }
            // Check activity mode
            if (_activityMode.isActive()) {
                _state = SafetyState::ACTIVITY_MODE;
                if (_eventCallback) _eventCallback(_state, "Activity mode active");
            }
            break;

        case SafetyState::SOS_COUNTDOWN:
            // Countdown before auto-SOS
            if (millis() - _countdownStartMs >= _countdownDurationMs) {
                // Countdown expired — trigger SOS
                triggerSOS(_countdownReason);
            }
            break;

        case SafetyState::SOS_ACTIVE:
            // Update SOS (live location, etc.)
            _sosManager.update();

            // Keep screen and sleep alive
            PowerManager::getInstance().preventSleep(true);

            // Auto-resolve: If ongoing heart rate anomaly has stopped (finger removed, HR stopped, or BPM normalized < 140 for 1.5s),
            // automatically return to the main screen rather than starting a new session!
            if (_sosManager.getReason() && strcmp(_sosManager.getReason(), "HR_ANOMALY") == 0) {
                HeartRateData hrData = sensors.getHeartRate().getData();
                if (!hrData.fingerPresent || hrData.bpm <= 0 || hrData.bpm < HR_ANOMALY_HIGH_BPM) {
                    if (_hrNormalStartMs == 0) _hrNormalStartMs = millis();
                    if (millis() - _hrNormalStartMs >= 1500) {
                        _hrNormalStartMs = 0;
                        Serial.println("[SAFE] ✅ Ongoing HR anomaly resolved (heart rate stopped/normalized) -> Returning to main screen");
                        stopSOS();
                    }
                } else {
                    _hrNormalStartMs = 0;
                }
            }
            break;

        case SafetyState::ACTIVITY_MODE:
            if (!_activityMode.isActive()) {
                _state = SafetyState::IDLE;
                if (_eventCallback) _eventCallback(_state, "Activity mode ended");
            }
            // Still check for fall/anomaly during activity mode
            // but with higher thresholds (handled by individual detectors)
            break;
    }
}

void SafetyEngine::triggerSOS(const char* reason) {
    if (_state == SafetyState::SOS_ACTIVE) return; // Already active

    // Cooldown check after stopping SOS: prevent starting a new session right after stopping
    if (_lastStopMs > 0 && (millis() - _lastStopMs < SOS_COOLDOWN_MS)) {
        Serial.printf("[SAFE] Cooldown active (%ums left) — rejecting trigger: %s\n",
                      (unsigned int)(SOS_COOLDOWN_MS - (millis() - _lastStopMs)), reason);
        return;
    }

    Serial.printf("[SAFE] 🚨 SOS TRIGGERED: %s\n", reason);

    _state = SafetyState::SOS_ACTIVE;
    _sosManager.trigger(reason);

    // Strong haptic feedback
    Vibration::getInstance().play(HapticPattern::SOS_CONFIRM);

    // Prevent sleep
    PowerManager::getInstance().preventSleep(true);

    if (_eventCallback) _eventCallback(_state, reason);
}

void SafetyEngine::stopSOS() {
    if (_state != SafetyState::SOS_ACTIVE && _state != SafetyState::SOS_COUNTDOWN) return;

    Serial.println("[SAFE] 🛑 SOS STOPPED -> Returning to main screen");

    _sosManager.stop();
    _state = SafetyState::IDLE;
    _lastStopMs = millis();
    _hrNormalStartMs = 0;

    // Reset anomaly detectors and enforce cooldown so it cannot immediately re-trigger
    _hrAnomaly.reset();
    _fallDetector.reset();

    // Explicitly return to the main watchface screen
    UIManager::getInstance().showSOSOverlay(false);
    UIManager::getInstance().showScreen(Screen::WATCHFACE);

    // Allow sleep again
    PowerManager::getInstance().preventSleep(false);

    Vibration::getInstance().play(HapticPattern::SOS_STOP);

    if (_eventCallback) _eventCallback(_state, "SOS stopped");
}

void SafetyEngine::startActivityMode(uint32_t durationSeconds) {
    _activityMode.start(durationSeconds);
    _state = SafetyState::ACTIVITY_MODE;
    Vibration::getInstance().play(HapticPattern::ACTIVITY_ON);
    BleService::getInstance().send("ACTIVITY:ENABLED");

    if (_eventCallback) _eventCallback(_state, "Activity mode started");
}

void SafetyEngine::stopActivityMode() {
    _activityMode.stop();
    _state = SafetyState::IDLE;
    Vibration::getInstance().play(HapticPattern::ACTIVITY_OFF);
    BleService::getInstance().send("ACTIVITY:DISABLED");

    if (_eventCallback) _eventCallback(_state, "Activity mode stopped");
}

void SafetyEngine::cancelCountdown() {
    if (_state == SafetyState::SOS_COUNTDOWN) {
        Serial.println("[SAFE] Countdown cancelled by user");
        _state = SafetyState::IDLE;
        _fallDetector.reset();
        _hrAnomaly.reset();
        Vibration::getInstance().play(HapticPattern::SUCCESS);

        if (_eventCallback) _eventCallback(_state, "Countdown cancelled");
    }
}

void SafetyEngine::_processGestureAction(int tapCount, bool longPress) {
    if (longPress) {
        // Long press → Stop SOS Anyhow
        Serial.println("[SAFETY] 🚨 Long press confirmed -> Stopping SOS ANYHOW");
        stopSOS();
        return;
    }

    // CRITICAL: When SOS is active or counting down, NO other taps or gestures are registered!
    if (_state == SafetyState::SOS_ACTIVE || _state == SafetyState::SOS_COUNTDOWN) {
        Serial.println("[SAFETY] ℹ️ Tap ignored — SOS active (Hold to cancel)");
        return;
    }

    switch (tapCount) {
        case GESTURE_ACTION_SWIPE_LEFT: // Swipe Right-to-Left -> Next Screen
            Serial.println("[SAFETY] ➡️ Swipe Right-to-Left -> Next Screen");
            UIManager::getInstance().nextScreen();
            break;

        case GESTURE_ACTION_SWIPE_RIGHT: // Swipe Left-to-Right -> Prev Screen
            Serial.println("[SAFETY] ⬅️ Swipe Left-to-Right -> Prev Screen");
            UIManager::getInstance().prevScreen();
            break;

        case 1: // 1 tap -> Advance to next screen
            if (!_activityMode.isActive()) {
                Serial.println("[SAFETY] 1 tap -> Next Screen");
                UIManager::getInstance().nextScreen();
            }
            break;

        case GESTURE_TAP_SOS: // 2 taps → SOS
            Serial.println("[SAFETY] 🚨 2 taps -> Triggering SOS!");
            triggerSOS("GESTURE_2TAP");
            Vibration::getInstance().play(HapticPattern::SOS_CONFIRM);
            break;

        case GESTURE_TAP_ACTIVITY_OFF: // 3 taps → Activity OFF
            if (_activityMode.isActive()) {
                Serial.println("[SAFETY] 3 taps -> Stopping activity mode");
                stopActivityMode();
                Vibration::getInstance().play(HapticPattern::ACTIVITY_OFF);
            }
            break;

        case GESTURE_TAP_ACTIVITY_ON: // 4 taps → Activity ON
            if (!_activityMode.isActive()) {
                uint32_t dur = _activityMode.getDefaultDurationSeconds();
                Serial.printf("[SAFETY] 4 taps -> Starting activity mode (%u s)\n", dur);
                startActivityMode(dur);
                Vibration::getInstance().play(HapticPattern::ACTIVITY_ON);
            }
            break;

        default:
            break;
    }
}

void SafetyEngine::_handleFallDetection() {
    Serial.println("[SAFE] ⚠️ Fall detected — starting countdown");
    _state = SafetyState::SOS_COUNTDOWN;
    _countdownStartMs = millis();
    _countdownDurationMs = SOS_FALL_COUNTDOWN_MS;
    _countdownReason = "FALL_DETECTED";

    Vibration::getInstance().play(HapticPattern::ALARM);

    if (_eventCallback) _eventCallback(_state, "Fall detected! SOS in 10s");
}

void SafetyEngine::_handleHRAnomaly() {
    // Disabled: Heart rate anomaly triggering SOS removed per request
}
