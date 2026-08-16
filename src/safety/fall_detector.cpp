// ============================================================================
// Suraksha+ Watch OS — Fall Detector Implementation
// ============================================================================
#include "fall_detector.h"

void FallDetector::init() {
    _state = FallState::MONITORING;
    _fallDetected = false;
    Serial.println("[FALL] Fall detector initialized");
}

void FallDetector::update(float accMagnitude, float motionScore) {
    switch (_state) {
        case FallState::MONITORING:
            // Detect high-G impact
            if (accMagnitude > IMU_FALL_IMPACT_G) {
                _impactTimeMs = millis();
                _state = FallState::IMPACT_DETECTED;
                Serial.printf("[FALL] ⚠️ Impact detected! (%.2fg)\n", accMagnitude);
            }
            break;

        case FallState::IMPACT_DETECTED:
            // After impact, check for extended stillness
            if (motionScore < IMU_FALL_STILL_THRESHOLD) {
                if (_stillStartMs == 0) {
                    _stillStartMs = millis();
                }

                // Check if still long enough
                if (millis() - _stillStartMs >= IMU_FALL_STILL_DURATION_MS) {
                    _state = FallState::FALL_CONFIRMED;
                    _fallDetected = true;
                    Serial.println("[FALL] 🚨 FALL CONFIRMED (impact + stillness)");
                }
            } else {
                // Movement detected after impact — reset
                _stillStartMs = 0;
            }

            // Timeout: if no stillness within 10s of impact, reset
            if (millis() - _impactTimeMs > 10000) {
                _state = FallState::MONITORING;
                _stillStartMs = 0;
                Serial.println("[FALL] Impact timeout — resuming monitoring");
            }
            break;

        case FallState::FALL_CONFIRMED:
            // Stay in confirmed state until explicitly reset
            break;
    }
}

void FallDetector::reset() {
    _state = FallState::MONITORING;
    _fallDetected = false;
    _stillStartMs = 0;
    Serial.println("[FALL] Fall detector reset");
}
