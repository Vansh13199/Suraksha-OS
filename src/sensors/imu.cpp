// ============================================================================
// Suraksha+ Watch OS — IMU Sensor Implementation
// MPU6050 driver ported from final.ino with step counter
// ============================================================================
#include "imu.h"

bool IMUSensor::init() {
    // Wake up MPU6050 (same as existing firmware)
    if (!_writeByte(MPU_REG_PWR_MGMT_1, 0x00)) {
        Serial.println("[IMU] ❌ MPU6050 not found!");
        _initialized = false;
        return false;
    }

    // Verify WHO_AM_I
    uint8_t whoAmI = 0;
    _readBytes(MPU_REG_WHO_AM_I, &whoAmI, 1);
    Serial.printf("[IMU] WHO_AM_I: 0x%02X (expected 0x68)\n", whoAmI);

    // Configure accelerometer: ±2g (default, matches existing firmware /16384.0)
    _writeByte(MPU_REG_ACCEL_CONFIG, 0x00);

    // Configure gyroscope: ±250°/s (default, matches existing firmware /131.0)
    _writeByte(MPU_REG_GYRO_CONFIG, 0x00);

    _initialized = true;
    Serial.println("[IMU] ✅ MPU6050 initialized");
    return true;
}

void IMUSensor::update() {
    if (!_initialized) return;

    float ax, ay, az, gx, gy, gz;
    if (!_readRaw(ax, ay, az, gx, gy, gz)) return;

    _data.ax = ax;
    _data.ay = ay;
    _data.az = az;
    _data.gx = gx;
    _data.gy = gy;
    _data.gz = gz;
    _data.timestamp = millis();

    // Calculate acceleration magnitude
    float accMag = sqrtf(ax * ax + ay * ay + az * az);
    _data.accMagnitude = accMag;

    // Calculate motion score (same as existing firmware)
    float accDelta = fabsf(accMag - 1.0f);
    float gyroMag = sqrtf(gx * gx + gy * gy + gz * gz);
    float scoreNow = accDelta + (gyroMag / 200.0f);
    _data.motionScore = _data.motionScore * IMU_MOTION_SMOOTHING + scoreNow * (1.0f - IMU_MOTION_SMOOTHING);

    // Step detection
    _detectStep(accMag);
}

void IMUSensor::_detectStep(float accMag) {
    uint32_t now = millis();

    // Simple peak detection algorithm
    // A step is detected when acceleration crosses the threshold going down
    if (!_stepPeakDetected && accMag > IMU_STEP_THRESHOLD_G) {
        _stepPeakDetected = true;
    } else if (_stepPeakDetected && accMag < 1.0f) {
        // Crossed back below 1g — count as a step
        if (now - _lastStepMs > MIN_STEP_INTERVAL_MS) {
            _data.steps++;
            _lastStepMs = now;
        }
        _stepPeakDetected = false;
    }

    _prevAccMag = accMag;
}

bool IMUSensor::_readRaw(float& ax, float& ay, float& az,
                         float& gx, float& gy, float& gz) {
    // Read 14 bytes starting from ACCEL_XOUT_H (same as existing firmware)
    uint8_t data[14];
    if (!_readBytes(MPU_REG_ACCEL_XOUT_H, data, 14)) return false;

    int16_t rawAx = (int16_t)((data[0] << 8) | data[1]);
    int16_t rawAy = (int16_t)((data[2] << 8) | data[3]);
    int16_t rawAz = (int16_t)((data[4] << 8) | data[5]);
    // data[6-7] = temperature (skipped)
    int16_t rawGx = (int16_t)((data[8] << 8) | data[9]);
    int16_t rawGy = (int16_t)((data[10] << 8) | data[11]);
    int16_t rawGz = (int16_t)((data[12] << 8) | data[13]);

    // Convert to physical units (same as existing firmware)
    ax = rawAx / 16384.0f;
    ay = rawAy / 16384.0f;
    az = rawAz / 16384.0f;
    gx = rawGx / 131.0f;
    gy = rawGy / 131.0f;
    gz = rawGz / 131.0f;

    return true;
}

bool IMUSensor::_writeByte(uint8_t reg, uint8_t data) {
    Wire.beginTransmission(MPU6050_I2C_ADDR);
    Wire.write(reg);
    Wire.write(data);
    return (Wire.endTransmission() == 0);
}

bool IMUSensor::_readBytes(uint8_t startReg, uint8_t* buf, uint8_t len) {
    Wire.beginTransmission(MPU6050_I2C_ADDR);
    Wire.write(startReg);
    if (Wire.endTransmission(false) != 0) return false;
    uint8_t got = Wire.requestFrom((int)MPU6050_I2C_ADDR, (int)len);
    if (got != len) return false;
    for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
}
