// ============================================================================
// Suraksha+ Watch OS — Sensor Manager Implementation
// ============================================================================
#include "sensor_manager.h"
#include "../config/hardware_config.h"
#include "../config/app_config.h"

SensorManager& SensorManager::getInstance() {
    static SensorManager instance;
    return instance;
}

void SensorManager::init() {
    Serial.println("[SENS] Initializing sensor manager...");

    // Create mutex for thread-safe data access
    _mutex = xSemaphoreCreateMutex();

    // Initialize I2C bus for sensors
    Wire.begin(PIN_SENSOR_SDA, PIN_SENSOR_SCL);
    Wire.setClock(I2C_SPEED);

    // Initialize individual sensors
    _hrAvailable = _hr.init();
    _imuAvailable = _imu.init();
    _battery.init();
    _gps.init();

    Serial.printf("[SENS] ✅ Sensors: HR=%s, IMU=%s, BAT=OK, GPS=%s\n",
                  _hrAvailable ? "OK" : "FAIL",
                  _imuAvailable ? "OK" : "FAIL",
                  GPS_HARDWARE_ENABLED ? "HW" : "BLE");
}

void SensorManager::update() {
    uint32_t now = millis();

    // Determine polling rates based on power mode
    uint32_t hrInterval, imuInterval, batInterval;

    switch (_powerMode) {
        case SensorPowerMode::ACTIVE:
            hrInterval = 0;     // Continuous
            imuInterval = 20;   // 50Hz
            batInterval = BATTERY_UPDATE_INTERVAL_MS;
            break;
        case SensorPowerMode::POWER_SAVE:
            hrInterval = 5000;  // Every 5 seconds
            imuInterval = 100;  // 10Hz
            batInterval = BATTERY_LP_UPDATE_MS;
            break;
        case SensorPowerMode::SLEEP:
            hrInterval = 0xFFFFFFFF; // Disabled
            imuInterval = 1000;      // 1Hz (tap detection only)
            batInterval = 300000;    // 5 minutes
            break;
    }

    // Update heart rate sensor
    if (_hrAvailable && (now - _lastHrUpdate >= hrInterval)) {
        _lastHrUpdate = now;
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10))) {
            _hr.update();
            xSemaphoreGive(_mutex);
        }
    }

    // Update IMU
    if (_imuAvailable && (now - _lastImuUpdate >= imuInterval)) {
        _lastImuUpdate = now;
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10))) {
            _imu.update();
            xSemaphoreGive(_mutex);
        }
    }

    // Update battery
    if (now - _lastBatUpdate >= batInterval) {
        _lastBatUpdate = now;
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10))) {
            _battery.update();
            xSemaphoreGive(_mutex);
        }
    }

    // Update GPS (hardware only — BLE updates come via setPhoneGPS)
    _gps.update();
}

void SensorManager::setPowerMode(SensorPowerMode mode) {
    _powerMode = mode;
    Serial.printf("[SENS] Power mode: %d\n", (int)mode);
}

// Thread-safe accessors
float SensorManager::getBPM() const {
    if (!_hrAvailable) return 0.0f;
    return _hr.getData().bpm;
}

float SensorManager::getMotionScore() const {
    if (!_imuAvailable) return 0.0f;
    return _imu.getData().motionScore;
}

uint32_t SensorManager::getSteps() const {
    if (!_imuAvailable) return 0;
    return _imu.getData().steps;
}

uint8_t SensorManager::getBatteryPercent() const {
    return _battery.getData().percentage;
}

bool SensorManager::isBatteryCharging() const {
    return _battery.getData().isCharging;
}

bool SensorManager::hasGPSFix() const {
    return _gps.getData().hasFix;
}

double SensorManager::getLatitude() const {
    return _gps.getData().latitude;
}

double SensorManager::getLongitude() const {
    return _gps.getData().longitude;
}
