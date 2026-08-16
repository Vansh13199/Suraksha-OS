// ============================================================================
// Suraksha+ Watch OS — BLE Protocol Implementation
// ============================================================================
#include "ble_protocol.h"
#include "../platform/storage.h"
#include "../sensors/sensor_manager.h"
#include "../safety/safety_engine.h"
#include "../drivers/vibration.h"
#include "../config/app_config.h"
#include <cstring>

BleProtocol& BleProtocol::getInstance() {
    static BleProtocol instance;
    return instance;
}

void BleProtocol::init() {
    Serial.println("[PROT] BLE protocol handler initialized");
}

void BleProtocol::handleMessage(const char* data, size_t len) {
    if (len == 0) return;

    // Match message prefix (backward-compatible with existing firmware)
    if (strncmp(data, BLE_MSG_SET_PHONE, strlen(BLE_MSG_SET_PHONE)) == 0) {
        _handleSetPhone(data);
    }
    else if (strncmp(data, BLE_MSG_ACTIVITY_ON, strlen(BLE_MSG_ACTIVITY_ON)) == 0 ||
             strncmp(data, BLE_MSG_ACTIVITY_OFF, strlen(BLE_MSG_ACTIVITY_OFF)) == 0) {
        _handleActivity(data);
    }
    else if (strncmp(data, BLE_MSG_TIME_SYNC, strlen(BLE_MSG_TIME_SYNC)) == 0) {
        _handleTimeSync(data);
    }
    else if (strncmp(data, BLE_MSG_NOTIFICATION, strlen(BLE_MSG_NOTIFICATION)) == 0) {
        _handleNotification(data);
    }
    else if (strncmp(data, BLE_MSG_GPS_DATA, strlen(BLE_MSG_GPS_DATA)) == 0) {
        _handleGPSData(data);
    }
    else if (strncmp(data, BLE_MSG_SETTINGS, strlen(BLE_MSG_SETTINGS)) == 0) {
        _handleSettings(data);
    }
    else if (strncmp(data, BLE_MSG_AI_RESPONSE, strlen(BLE_MSG_AI_RESPONSE)) == 0) {
        _handleAIResponse(data);
    }
    else if (strncmp(data, BLE_MSG_DND_MODE, strlen(BLE_MSG_DND_MODE)) == 0) {
        _handleDND(data);
    }
    else if (strncmp(data, BLE_MSG_SOS_CMD, strlen(BLE_MSG_SOS_CMD)) == 0) {
        _handleSOSCommand(data);
    }
    else {
        Serial.printf("[PROT] Unknown message: %s\n", data);
    }
}

void BleProtocol::_handleSetPhone(const char* data) {
    // SET_PHONE:+91XXXXXXXXXX (same as existing firmware)
    const char* phone = data + strlen(BLE_MSG_SET_PHONE);
    char cleanPhone[20];
    strncpy(cleanPhone, phone, sizeof(cleanPhone) - 1);
    cleanPhone[sizeof(cleanPhone) - 1] = '\0';

    // Trim whitespace
    int len = strlen(cleanPhone);
    while (len > 0 && (cleanPhone[len-1] == ' ' || cleanPhone[len-1] == '\n' || cleanPhone[len-1] == '\r')) {
        cleanPhone[--len] = '\0';
    }

    Storage::getInstance().putString(NVS_KEY_PHONE, cleanPhone);
    Serial.printf("[PROT] ✅ Identity synced: %s\n", cleanPhone);
    Vibration::getInstance().play(HapticPattern::IDENTITY_SYNC);
}

void BleProtocol::_handleActivity(const char* data) {
    SafetyEngine& safety = SafetyEngine::getInstance();

    if (strncmp(data, BLE_MSG_ACTIVITY_ON, strlen(BLE_MSG_ACTIVITY_ON)) == 0) {
        // Parse duration: ACTIVITY:1,SECONDS:N
        uint32_t duration = ACTIVITY_DEFAULT_DURATION_S;
        const char* secStr = strstr(data, "SECONDS:");
        if (secStr) {
            duration = atoi(secStr + 8);
        } else {
            const char* comma = strchr(data, ',');
            if (comma) {
                duration = atoi(comma + 1);
            }
        }
        safety.startActivityMode(duration);
    }
    else if (strncmp(data, BLE_MSG_ACTIVITY_OFF, strlen(BLE_MSG_ACTIVITY_OFF)) == 0) {
        safety.stopActivityMode();
    }
}

void BleProtocol::_handleTimeSync(const char* data) {
    // TIME:epoch_seconds
    const char* timeStr = data + strlen(BLE_MSG_TIME_SYNC);
    unsigned long epoch = strtoul(timeStr, nullptr, 10);
    if (epoch > 1000000000UL) {
        // Set system time
        struct timeval tv;
        tv.tv_sec = epoch;
        tv.tv_usec = 0;
        settimeofday(&tv, nullptr);
        Serial.printf("[PROT] ⏰ Time synced: %lu\n", epoch);
    }
}

void BleProtocol::_handleNotification(const char* data) {
    // NOTIF:type:title:body
    const char* payload = data + strlen(BLE_MSG_NOTIFICATION);
    Serial.printf("[PROT] 🔔 Notification: %s\n", payload);
    Vibration::getInstance().play(HapticPattern::NOTIFICATION);
    // TODO: Forward to notification manager for UI display
}

void BleProtocol::_handleGPSData(const char* data) {
    // GPS:lat,lon,alt,speed
    const char* payload = data + strlen(BLE_MSG_GPS_DATA);
    double lat = 0, lon = 0, alt = 0;
    float speed = 0;
    sscanf(payload, "%lf,%lf,%lf,%f", &lat, &lon, &alt, &speed);

    SensorManager::getInstance().getGPS().setPhoneGPS(lat, lon, alt, speed);
    Serial.printf("[PROT] 📍 Phone GPS: %.6f, %.6f\n", lat, lon);
}

void BleProtocol::_handleSettings(const char* data) {
    // SETTINGS:key:value
    const char* payload = data + strlen(BLE_MSG_SETTINGS);
    Serial.printf("[PROT] ⚙️ Settings: %s\n", payload);
    // TODO: Parse and apply settings
}

void BleProtocol::_handleAIResponse(const char* data) {
    // AI:R:response_text
    const char* response = data + strlen(BLE_MSG_AI_RESPONSE);
    Serial.printf("[PROT] 🤖 AI Response: %s\n", response);
    // TODO: Forward to AI assistant screen
}

void BleProtocol::_handleDND(const char* data) {
    // DND:1 or DND:0
    const char* val = data + strlen(BLE_MSG_DND_MODE);
    bool dnd = (val[0] == '1');
    Storage::getInstance().putBool(NVS_KEY_DND_MODE, dnd);
    Serial.printf("[PROT] 🔕 DND: %s\n", dnd ? "ON" : "OFF");
}

void BleProtocol::_handleSOSCommand(const char* data) {
    SafetyEngine& safety = SafetyEngine::getInstance();

    if (strstr(data, "START") != nullptr) {
        safety.triggerSOS("PHONE_COMMAND");
    } else if (strstr(data, "STOP") != nullptr) {
        safety.stopSOS();
    }
}
