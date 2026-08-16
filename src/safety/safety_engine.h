// ============================================================================
// Suraksha+ Watch OS — Safety Engine
// Central coordinator for all safety functionality
// Highest priority — independent of UI
// ============================================================================
#ifndef SAFETY_ENGINE_H
#define SAFETY_ENGINE_H

#include <Arduino.h>
#include "sos_manager.h"
#include "gesture_detector.h"
#include "fall_detector.h"
#include "hr_anomaly.h"
#include "activity_mode.h"

enum class SafetyState {
    IDLE,               // Normal watch operation
    SOS_COUNTDOWN,      // Fall/anomaly detected — countdown before SOS
    SOS_ACTIVE,         // SOS is active and transmitting
    ACTIVITY_MODE       // Activity monitoring active
};

// Callback for UI updates
typedef void (*SafetyEventCallback)(SafetyState state, const char* message);

class SafetyEngine {
public:
    static SafetyEngine& getInstance();

    void init();
    void update();  // Called from high-priority FreeRTOS task

    // State
    SafetyState getState() const { return _state; }
    bool isSOSActive() const { return _state == SafetyState::SOS_ACTIVE; }
    bool isActivityMode() const { return _activityMode.isActive(); }

    // Manual SOS trigger/stop
    void triggerSOS(const char* reason = "MANUAL");
    void stopSOS();

    // Activity mode
    void startActivityMode(uint32_t durationSeconds);
    void stopActivityMode();

    // Countdown cancel (user tapped during fall detection countdown)
    void cancelCountdown();

    // Get sub-managers
    SOSManager& getSOSManager() { return _sosManager; }
    GestureDetector& getGestureDetector() { return _gestureDetector; }
    FallDetector& getFallDetector() { return _fallDetector; }
    HRAnomaly& getHRAnomaly() { return _hrAnomaly; }
    ActivityMode& getActivityMode() { return _activityMode; }

    // Event callback
    void setEventCallback(SafetyEventCallback cb) { _eventCallback = cb; }

private:
    SafetyEngine() = default;

    void _processGestureAction(int tapCount, bool longPress);
    void _handleFallDetection();
    void _handleHRAnomaly();

    SafetyState _state = SafetyState::IDLE;
    SOSManager _sosManager;
    GestureDetector _gestureDetector;
    FallDetector _fallDetector;
    HRAnomaly _hrAnomaly;
    ActivityMode _activityMode;

    SafetyEventCallback _eventCallback = nullptr;

    // Countdown state
    uint32_t _countdownStartMs = 0;
    uint32_t _countdownDurationMs = 0;
    const char* _countdownReason = nullptr;
};

#endif // SAFETY_ENGINE_H
