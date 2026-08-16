// ============================================================================
// Suraksha+ Watch OS — Phone Bridge Implementation
// ============================================================================
#include "phone_bridge.h"
#include "ble_service.h"
#include "../config/ble_config.h"
#include "../sensors/sensor_manager.h"

PhoneBridge& PhoneBridge::getInstance() {
    static PhoneBridge instance;
    return instance;
}

void PhoneBridge::init() {
    Serial.println("[BRIDGE] Phone bridge initialized");
}

void PhoneBridge::update() {
    BleService& ble = BleService::getInstance();
    if (!ble.isConnected()) return;

    uint32_t now = millis();

    // Periodic health data push
    if (now - _lastHealthPushMs >= HEALTH_PUSH_INTERVAL_MS) {
        _lastHealthPushMs = now;
        sendHealthData();
    }

    // Periodic battery status push
    if (now - _lastBatteryPushMs >= BATTERY_PUSH_INTERVAL_MS) {
        _lastBatteryPushMs = now;
        sendBatteryStatus();
    }

    _lastSyncMs = now;
}

void PhoneBridge::sendHealthData() {
    SensorManager& sensors = SensorManager::getInstance();
    BleService& ble = BleService::getInstance();

    if (!ble.isConnected()) return;

    float bpm = sensors.getBPM();
    uint32_t steps = sensors.getSteps();
    float motion = sensors.getMotionScore();

    // HEALTH:bpm,spo2,steps,calories
    ble.sendFormatted("%s%.1f,0,%d,0", BLE_MSG_HEALTH_DATA, bpm, steps);
}

void PhoneBridge::sendBatteryStatus() {
    SensorManager& sensors = SensorManager::getInstance();
    BleService& ble = BleService::getInstance();

    if (!ble.isConnected()) return;

    uint8_t pct = sensors.getBatteryPercent();
    bool charging = sensors.isBatteryCharging();

    // BATTERY:pct,charging
    ble.sendFormatted("%s%d,%d", BLE_MSG_BATTERY_STATUS, pct, charging ? 1 : 0);
}

void PhoneBridge::sendSensorData() {
    SensorManager& sensors = SensorManager::getInstance();
    BleService& ble = BleService::getInstance();

    if (!ble.isConnected()) return;

    IMUData imu = sensors.getIMU().getData();
    ble.sendFormatted("%s%.2f,%.2f,%.2f,%.1f,%.1f,%.1f",
                      BLE_MSG_SENSOR_DATA,
                      imu.ax, imu.ay, imu.az,
                      imu.gx, imu.gy, imu.gz);
}

void PhoneBridge::requestGPS() {
    BleService& ble = BleService::getInstance();
    if (ble.isConnected()) {
        ble.send(BLE_MSG_GPS_REQUEST);
    }
}

void PhoneBridge::findPhone() {
    BleService& ble = BleService::getInstance();
    if (ble.isConnected()) {
        ble.send(BLE_MSG_FIND_PHONE);
        Serial.println("[BRIDGE] 📲 Find phone command sent");
    }
}

void PhoneBridge::sendAIQuery(const char* query) {
    BleService& ble = BleService::getInstance();
    if (ble.isConnected()) {
        ble.sendFormatted("%s%s", BLE_MSG_AI_QUERY, query);
    }
}
