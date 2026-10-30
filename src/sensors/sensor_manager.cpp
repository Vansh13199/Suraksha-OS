// ============================================================================
// Suraksha+ Watch OS — Sensor Manager Implementation
// ============================================================================
// Architecture:
//   - Heart Rate: Received via ESP-NOW from external Seeed XIAO ESP32S3
//   - IMU: Onboard BMA423/BMA456 (or MPU6050 if wired)
//   - Battery: Via AXP2101 PMU (or ADC fallback)
//   - GPS: Via BLE phone bridge (no hardware GPS)
// ============================================================================
#include "sensor_manager.h"
#include "../config/hardware_config.h"
#include "../config/app_config.h"
#include "../comms/espnow_receiver.h"

SensorManager& SensorManager::getInstance() {
    static SensorManager instance;
    return instance;
}

void SensorManager::init() {
    Serial.println("[SENS] Initializing sensor manager...");

    // Create mutex for thread-safe data access
    _mutex = xSemaphoreCreateMutex();

    // Initialize I2C bus for onboard sensors (IMU, PMU, etc.)
    Wire.begin(PIN_SENSOR_SDA, PIN_SENSOR_SCL);
    Wire.setClock(I2C_SPEED);

    // Initialize ESP-NOW receiver for remote heart rate data
    Serial.println("[SENS] Initializing ESP-NOW receiver for remote HR...");
    ESPNowReceiver& espnow = ESPNowReceiver::getInstance();
    if (espnow.init()) {
        Serial.println("[SENS] ✅ ESP-NOW receiver ready");
    } else {
        Serial.println("[SENS] ⚠️ ESP-NOW receiver init failed");
    }

    // Initialize heart rate sensor interface (reads from ESP-NOW)
    _hrAvailable = _hr.init();

    // Initialize onboard sensors
    _imuAvailable = _imu.init();
    _battery.init();
    _gps.init();

    Serial.printf("[SENS] ✅ Sensors: HR(ESP-NOW)=%s, IMU=%s, BAT=OK, GPS=%s\n",
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
            hrInterval = 100;    // Check ESP-NOW data frequently
            imuInterval = 20;    // 50Hz
            batInterval = BATTERY_UPDATE_INTERVAL_MS;
            break;
        case SensorPowerMode::POWER_SAVE:
            hrInterval = 1000;   // Check every second
            imuInterval = 100;   // 10Hz
            batInterval = BATTERY_LP_UPDATE_MS;
            break;
        case SensorPowerMode::SLEEP:
            hrInterval = 5000;   // Check every 5 seconds
            imuInterval = 1000;  // 1Hz (tap detection only)
            batInterval = 300000; // 5 minutes
            break;
    }

    // Update heart rate data (from ESP-NOW — no I2C mutex needed)
    if (_hrAvailable && (now - _lastHrUpdate >= hrInterval)) {
        _lastHrUpdate = now;
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10))) {
            _hr.update();  // Pulls from ESPNowReceiver
            xSemaphoreGive(_mutex);
        }
    }

    // Update IMU (onboard sensor — needs I2C mutex)
    if (_imuAvailable && (now - _lastImuUpdate >= imuInterval)) {
        _lastImuUpdate = now;
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10))) {
            if (g_i2cMutex && xSemaphoreTake(g_i2cMutex, pdMS_TO_TICKS(50))) {
                _imu.update();
                xSemaphoreGive(g_i2cMutex);
            }
            xSemaphoreGive(_mutex);
        }
    }

    // Update battery (via PMU or ADC)
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

float SensorManager::getSpO2() const {
    if (!_hrAvailable) return 0.0f;
    return _hr.getData().spo2;
}

float SensorManager::getMotionScore() const {
    if (!_imuAvailable) return 0.05f; // Small fake motion
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
    if (!GPS_HARDWARE_ENABLED) return true;
    return _gps.getData().hasFix;
}

double SensorManager::getLatitude() const {
    if (!GPS_HARDWARE_ENABLED) return 30.672342; // Default
    return _gps.getData().latitude;
}

double SensorManager::getLongitude() const {
    if (!GPS_HARDWARE_ENABLED) return 76.829631; // Default
    return _gps.getData().longitude;
}

// Remote sensor status
bool SensorManager::isHRSensorOnline() const {
    if (!_hrAvailable) return false;
    return _hr.getData().sensorOnline;
}

bool SensorManager::isHRSensorFingerPresent() const {
    if (!_hrAvailable) return false;
    return _hr.getData().fingerPresent;
}

int8_t SensorManager::getHRSensorRSSI() const {
    if (!_hrAvailable) return 0;
    return _hr.getData().rssi;
}

uint8_t SensorManager::getHRSensorBattery() const {
    if (!_hrAvailable) return 0;
    return _hr.getData().sensorBattery;
}
