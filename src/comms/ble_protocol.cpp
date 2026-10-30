// ============================================================================
// Suraksha+ Watch OS — BLE Protocol Implementation
// ============================================================================
#include "ble_protocol.h"
#include "ble_service.h"
#include "../platform/storage.h"
#include "../sensors/sensor_manager.h"
#include "../safety/safety_engine.h"
#include "../drivers/vibration.h"
#include "../ui/ui_manager.h"
#include "../config/app_config.h"
#include <cstring>
#include <cctype>

BleProtocol& BleProtocol::getInstance() {
    static BleProtocol instance;
    return instance;
}

void BleProtocol::init() {
    BleService::getInstance().onReceive([](const char* data, size_t len) {
        BleProtocol::getInstance().handleMessage(data, len);
    });
    Serial.println("[PROT] ✅ BLE protocol handler initialized and wired to BleService");
}

void BleProtocol::handleMessage(const char* data, size_t len) {
    if (len == 0 || data == nullptr) return;

    Serial.printf("[PROT] 📩 Incoming BLE RX (%u bytes): %s\n", (unsigned int)len, data);

    // Match message prefix (backward-compatible with existing firmware)
    if (strncmp(data, BLE_MSG_SET_PHONE, strlen(BLE_MSG_SET_PHONE)) == 0) {
        _handleSetPhone(data);
    }
    else if (strncmp(data, BLE_MSG_ACTIVITY_CMD, strlen(BLE_MSG_ACTIVITY_CMD)) == 0 ||
             strncmp(data, "TIMER:", 6) == 0 ||
             strncmp(data, "SET_TIMER:", 10) == 0 ||
             strncmp(data, "DURATION:", 9) == 0) {
        _handleActivity(data);
    }
    else if (strncmp(data, BLE_MSG_TIME_SYNC, strlen(BLE_MSG_TIME_SYNC)) == 0) {
        _handleTimeSync(data);
    }
    else if (strncmp(data, BLE_MSG_NOTIFICATION, strlen(BLE_MSG_NOTIFICATION)) == 0) {
        _handleNotification(data);
    }
    else if (strncmp(data, BLE_MSG_GPS_DATA, strlen(BLE_MSG_GPS_DATA)) == 0 ||
             strncmp(data, "LAT:", 4) == 0) {
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
        // Fallback: check if the string contains ACTIVITY or TIMER anywhere
        if (strstr(data, "ACTIVITY") != nullptr || strstr(data, "TIMER") != nullptr) {
            _handleActivity(data);
        } else {
            Serial.printf("[PROT] Unknown message: %s\n", data);
        }
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
    Serial.printf("[PROT] 🏃 Activity command received: %s\n", data);

    // 1. Check if STOP / OFF / DISABLED command
    if (strstr(data, "ACTIVITY:0") != nullptr ||
        strstr(data, "OFF") != nullptr ||
        strstr(data, "DISABLED") != nullptr ||
        strstr(data, "STOP") != nullptr) {
        safety.stopActivityMode();
        return;
    }

    // 2. Parse duration/time if present
    // Supported formats:
    // ACTIVITY:TIME:600, ACTIVITY:TIMER:600, ACTIVITY:DURATION:600, ACTIVITY:SECONDS:600, ACTIVITY:SET:600
    // ACTIVITY:1,SECONDS:600, ACTIVITY:1,TIME:600, ACTIVITY:1,600
    // TIMER:600, TIMER:ACTIVITY:600
    // Suffixes: 10MIN, 10m -> 600s, 1H -> 3600s
    uint32_t parsedDuration = 0;
    const char* p = nullptr;

    if ((p = strstr(data, "SECONDS:")) != nullptr) {
        p += 8;
    } else if ((p = strstr(data, "SEC:")) != nullptr) {
        p += 4;
    } else if ((p = strstr(data, "TIMER:")) != nullptr) {
        p += 6;
        if (strncmp(p, "ACTIVITY:", 9) == 0) p += 9;
    } else if ((p = strstr(data, "DURATION:")) != nullptr) {
        p += 9;
    } else if ((p = strstr(data, "TIME:")) != nullptr) {
        p += 5;
    } else if ((p = strstr(data, "SET:")) != nullptr) {
        p += 4;
    } else if ((p = strstr(data, "UPDATE:")) != nullptr) {
        p += 7;
    } else if ((p = strstr(data, "CHANGE:")) != nullptr) {
        p += 7;
    } else if ((p = strchr(data, ',')) != nullptr) {
        p += 1;
        if (strncmp(p, "SECONDS:", 8) == 0) p += 8;
        else if (strncmp(p, "TIME:", 5) == 0) p += 5;
    } else if (strncmp(data, "TIMER:", 6) == 0) {
        p = data + 6;
    } else if (strncmp(data, "ACTIVITY:", 9) == 0) {
        // Direct number after ACTIVITY: e.g. ACTIVITY:300 or ACTIVITY:600
        const char* afterAct = data + 9;
        while (*afterAct == ' ') afterAct++;
        if (isdigit(*afterAct) && *afterAct != '0' && *afterAct != '1') {
            p = afterAct;
        }
    }

    if (p != nullptr) {
        while (*p == ' ') p++;
        char* endPtr = nullptr;
        long val = strtol(p, &endPtr, 10);
        if (val > 0) {
            while (endPtr && *endPtr == ' ') endPtr++;
            if (endPtr && (*endPtr == 'm' || *endPtr == 'M')) {
                parsedDuration = (uint32_t)(val * 60);
            } else if (endPtr && (*endPtr == 'h' || *endPtr == 'H')) {
                parsedDuration = (uint32_t)(val * 3600);
            } else {
                parsedDuration = (uint32_t)val;
            }
        }
    }

    // If duration was parsed, apply it to the activity mode
    if (parsedDuration > 0) {
        safety.setActivityDuration(parsedDuration);
        Serial.printf("[PROT] ⏱ Activity mode timer set to %u s\n", parsedDuration);
    }

    // 3. Start or re-sync Activity Mode & immediately display on watchface
    bool shouldStartOrResync = (strstr(data, "ACTIVITY:1") != nullptr ||
                                strstr(data, "START") != nullptr ||
                                strstr(data, "ENABLED") != nullptr ||
                                strstr(data, "ON") != nullptr ||
                                parsedDuration > 0);

    if (shouldStartOrResync) {
        uint32_t dur = (parsedDuration > 0) ? parsedDuration : safety.getActivityMode().getDefaultDurationSeconds();
        safety.startActivityMode(dur);
        UIManager::getInstance().showScreen(Screen::WATCHFACE);
        Serial.printf("[PROT] 🏃 Activity Mode running with synced %u s timer\n", dur);
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
    double lat = 0, lon = 0, alt = 0;
    float speed = 0;

    if (strncmp(data, "LAT:", 4) == 0) {
        // LAT:30.6720863,LON:76.8296332
        sscanf(data, "LAT:%lf,LON:%lf", &lat, &lon);
    } else {
        // GPS:lat,lon,alt,speed
        const char* payload = data + strlen(BLE_MSG_GPS_DATA);
        sscanf(payload, "%lf,%lf,%lf,%f", &lat, &lon, &alt, &speed);
    }

    SensorManager::getInstance().getGPS().setPhoneGPS(lat, lon, alt, speed);
    Serial.printf("[PROT] 📍 Phone GPS: %.6f, %.6f\n", lat, lon);
}

void BleProtocol::_handleSettings(const char* data) {
    // SETTINGS:key:value
    const char* payload = data + strlen(BLE_MSG_SETTINGS);
    Serial.printf("[PROT] ⚙️ Settings: %s\n", payload);

    if (strstr(payload, "ACT") != nullptr || strstr(payload, "TIMER") != nullptr || strstr(payload, "DUR") != nullptr) {
        _handleActivity(payload);
    }
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
