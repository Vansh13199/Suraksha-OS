// ============================================================================
// Suraksha+ Watch OS — Safety Engine Implementation
// ============================================================================
#include "safety_engine.h"
#include "../sensors/sensor_manager.h"
#include "../drivers/vibration.h"
#include "../platform/power_manager.h"
#include "../config/app_config.h"

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

    // Always update gesture detector (tap detection)
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
            // Check for HR anomaly
            if (_hrAnomaly.isAnomalyDetected() && !_activityMode.isActive()) {
                _handleHRAnomaly();
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
    if (_state != SafetyState::SOS_ACTIVE) return;

    Serial.println("[SAFE] SOS stopped");

    _sosManager.stop();
    _state = SafetyState::IDLE;

    // Allow sleep again
    PowerManager::getInstance().preventSleep(false);

    Vibration::getInstance().play(HapticPattern::SUCCESS);

    if (_eventCallback) _eventCallback(_state, "SOS stopped");
}

void SafetyEngine::startActivityMode(uint32_t durationSeconds) {
    _activityMode.start(durationSeconds);
    _state = SafetyState::ACTIVITY_MODE;
    Vibration::getInstance().play(HapticPattern::ACTIVITY_ON);

    if (_eventCallback) _eventCallback(_state, "Activity mode started");
}

void SafetyEngine::stopActivityMode() {
    _activityMode.stop();
    _state = SafetyState::IDLE;
    Vibration::getInstance().play(HapticPattern::ACTIVITY_OFF);

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
        // 5-second long press → Stop SOS
        if (_state == SafetyState::SOS_ACTIVE) {
            stopSOS();
        }
        return;
    }

    switch (tapCount) {
        case GESTURE_TAP_SOS: // 2 taps → SOS
            if (_state != SafetyState::SOS_ACTIVE) {
                triggerSOS("GESTURE_2TAP");
            }
            break;

        case GESTURE_TAP_ACTIVITY_OFF: // 3 taps → Activity OFF
            if (_activityMode.isActive()) {
                stopActivityMode();
            }
            break;

        case GESTURE_TAP_ACTIVITY_ON: // 4 taps → Activity ON
            if (!_activityMode.isActive()) {
                startActivityMode(ACTIVITY_DEFAULT_DURATION_S);
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
    Serial.println("[SAFE] ⚠️ HR anomaly detected");
    // Immediate SOS (matching existing firmware behavior)
    triggerSOS("HR_ANOMALY");
}
