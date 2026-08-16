// ============================================================================
// Suraksha+ Watch OS — IMU Sensor (MPU6050)
// Ported from existing firmware with step counter added
// ============================================================================
#ifndef IMU_H
#define IMU_H

#include <Arduino.h>
#include <Wire.h>
#include "../config/hardware_config.h"
#include "../config/app_config.h"

struct IMUData {
    float ax, ay, az;       // Acceleration (g)
    float gx, gy, gz;       // Gyroscope (degrees/s)
    float motionScore;      // Smoothed motion magnitude
    float accMagnitude;     // |acceleration| vector magnitude
    uint32_t steps;         // Step counter
    uint32_t timestamp;
};

class IMUSensor {
public:
    bool init();
    void update();          // Call at IMU polling rate
    IMUData getData() const { return _data; }
    float getMotionScore() const { return _data.motionScore; }
    uint32_t getSteps() const { return _data.steps; }
    void resetSteps() { _data.steps = 0; }

private:
    bool _readRaw(float& ax, float& ay, float& az,
                  float& gx, float& gy, float& gz);
    bool _writeByte(uint8_t reg, uint8_t data);
    bool _readBytes(uint8_t startReg, uint8_t* buf, uint8_t len);

    // Step detection
    void _detectStep(float accMag);

    IMUData _data = {};
    bool _initialized = false;

    // Step detection state
    float _prevAccMag = 1.0f;
    bool _stepPeakDetected = false;
    uint32_t _lastStepMs = 0;
    static const uint32_t MIN_STEP_INTERVAL_MS = 250; // Max ~4 steps/sec
};

#endif // IMU_H
