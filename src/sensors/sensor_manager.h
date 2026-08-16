// ============================================================================
// Suraksha+ Watch OS — Sensor Manager
// Unified orchestration for all sensors
// ============================================================================
#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include "heart_rate.h"
#include "imu.h"
#include "battery.h"
#include "gps.h"

enum class SensorPowerMode {
    ACTIVE,         // Full rate: HR=continuous, IMU=50Hz, Battery=30s
    POWER_SAVE,     // Reduced: HR=periodic, IMU=10Hz, Battery=60s
    SLEEP           // Minimal: HR=off, IMU=tap-only, Battery=5min
};

class SensorManager {
public:
    static SensorManager& getInstance();

    void init();
    void update();  // Called from sensor FreeRTOS task

    // Power mode
    void setPowerMode(SensorPowerMode mode);
    SensorPowerMode getPowerMode() const { return _powerMode; }

    // Direct sensor access
    HeartRateSensor& getHeartRate() { return _hr; }
    IMUSensor& getIMU() { return _imu; }
    BatterySensor& getBattery() { return _battery; }
    GPSSensor& getGPS() { return _gps; }

    // Convenience accessors (thread-safe)
    float getBPM() const;
    float getMotionScore() const;
    uint32_t getSteps() const;
    uint8_t getBatteryPercent() const;
    bool isBatteryCharging() const;
    bool hasGPSFix() const;
    double getLatitude() const;
    double getLongitude() const;

    // Sensor health
    bool isHeartRateAvailable() const { return _hrAvailable; }
    bool isIMUAvailable() const { return _imuAvailable; }

private:
    SensorManager() = default;

    HeartRateSensor _hr;
    IMUSensor _imu;
    BatterySensor _battery;
    GPSSensor _gps;

    SensorPowerMode _powerMode = SensorPowerMode::ACTIVE;
    bool _hrAvailable = false;
    bool _imuAvailable = false;

    uint32_t _lastHrUpdate = 0;
    uint32_t _lastImuUpdate = 0;
    uint32_t _lastBatUpdate = 0;

    // Mutex for thread-safe data access
    SemaphoreHandle_t _mutex = nullptr;
};

#endif // SENSOR_MANAGER_H
