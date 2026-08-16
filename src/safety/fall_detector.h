// ============================================================================
// Suraksha+ Watch OS — Fall Detector
// ============================================================================
#ifndef FALL_DETECTOR_H
#define FALL_DETECTOR_H

#include <Arduino.h>
#include "../config/app_config.h"

class FallDetector {
public:
    void init();
    void update(float accMagnitude, float motionScore);
    void reset();

    bool isFallDetected() const { return _fallDetected; }

private:
    enum class FallState {
        MONITORING,     // Normal — watching for impact
        IMPACT_DETECTED,// High-G detected — watching for stillness
        FALL_CONFIRMED  // Impact + stillness = fall
    };

    FallState _state = FallState::MONITORING;
    bool _fallDetected = false;
    uint32_t _impactTimeMs = 0;
    uint32_t _stillStartMs = 0;
};

#endif // FALL_DETECTOR_H
