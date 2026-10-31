// ============================================================================
// Suraksha+ Watch OS — SOS Routing Implementation
// ============================================================================
#include "sos_routing.h"
#include "ble_service.h"
#include "../config/app_config.h"
#include "../config/ble_config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

SOSRouting& SOSRouting::getInstance() {
    static SOSRouting instance;
    return instance;
}

#include "wifi_manager.h"

SOSRoute SOSRouting::getBestRoute() const {
    if (WiFi.status() == WL_CONNECTED) {
        return SOSRoute::WIFI_DIRECT;
    }
    if (BleService::getInstance().isConnected()) {
        return SOSRoute::BLE_TO_PHONE;
    }
    return SOSRoute::STORE_AND_FORWARD;
}

bool SOSRouting::sendSOS(const char* phoneNumber, const char* sessionId,
                         const char* lat, const char* lon, float heartRate) {
    Serial.printf("[ROUTE] 🚨 sendSOS called: phone=%s session=%s\n", phoneNumber, sessionId);

    // Priority 1: If BLE mobile is connected, dispatch to phone.
    // The phone manages the single canonical SOS session (AWS + SMS + Emergency Calling).
    // Do NOT send a second direct WiFi session to prevent duplicate sessions.
    if (BleService::getInstance().isConnected()) {
        Serial.println("[ROUTE] 📱 Mobile BLE connected -> Dispatching via Bluetooth (Phone manages session)");
        return _sendViaBLE(sessionId, lat, lon);
    }

    // Priority 2: Standalone Mode (No phone connected via BLE) -> Send directly to AWS via WiFi
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[ROUTE] 🌐 WiFi disconnected. Connecting for standalone SOS dispatch...");
        WifiManager::getInstance().connect(4000);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[ROUTE] 🌐 WiFi connected -> Sending SOS directly to AWS API");
        if (_sendViaWiFi(phoneNumber, sessionId, lat, lon, heartRate)) {
            Serial.println("[ROUTE] ✅ SOS successfully dispatched to AWS!");
            return true;
        }
    } else {
        Serial.println("[ROUTE] ⚠️ Neither Bluetooth nor WiFi connected — SOS queued locally");
    }

    return false;
}

bool SOSRouting::stopSOS(const char* sessionId) {
    Serial.printf("[ROUTE] 🛑 stopSOS called for session: %s\n", sessionId);

    // If BLE mobile is connected, notify mobile to stop the SOS session
    if (BleService::getInstance().isConnected()) {
        Serial.println("[ROUTE] 📱 Sending SOS:STOP to Mobile via Bluetooth");
        BleService::getInstance().send(BLE_MSG_SOS_STOP);
        return true;
    }

    // Standalone Mode: Send STOP directly to AWS via WiFi
    if (WiFi.status() != WL_CONNECTED) {
        WifiManager::getInstance().connect(3000);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[ROUTE] 🌐 Sending STOP to AWS via WiFi");
        char payload[128];
        snprintf(payload, sizeof(payload), "{\"sessionId\":\"%s\"}", sessionId);
        return _sendViaHTTP(AWS_SOS_STOP_URL, payload);
    }

    return false;
}

bool SOSRouting::sendLiveUpdate(const char* phoneNumber, const char* sessionId,
                                const char* lat, const char* lon, float heartRate) {
    if (BleService::getInstance().isConnected()) {
        BleService::getInstance().sendFormatted("SOS:UPDATE:%s,%s,%.1f",
                                                 lat, lon, heartRate);
        return true;
    } else if (WiFi.status() == WL_CONNECTED) {
        return _sendViaWiFi(phoneNumber, sessionId, lat, lon, heartRate);
    }
    return false;
}

bool SOSRouting::_sendViaBLE(const char* sessionId, const char* lat, const char* lon) {
    BleService& ble = BleService::getInstance();

    // Send SOS:START to phone — triggers SosProvider on the Flutter mobile app
    Serial.println("[ROUTE] 📱 Sending SOS:START to mobile via BLE");
    ble.send(BLE_MSG_SOS_START);

    return true;
}

bool SOSRouting::_sendViaWiFi(const char* phoneNumber, const char* sessionId,
                              const char* lat, const char* lon, float heartRate) {
    double latitude = (lat && strlen(lat) > 0) ? atof(lat) : FIXED_LATITUDE;
    double longitude = (lon && strlen(lon) > 0) ? atof(lon) : FIXED_LONGITUDE;
    if (latitude == 0.0) latitude = FIXED_LATITUDE;
    if (longitude == 0.0) longitude = FIXED_LONGITUDE;

    char payload[320];
    snprintf(payload, sizeof(payload),
        "{\"phoneNumber\":\"%s\",\"sessionId\":\"%s\",\"latitude\":%.6f,\"longitude\":%.6f,\"heartRate\":%.1f}",
        phoneNumber, sessionId, latitude, longitude, heartRate);

    return _sendViaHTTP(AWS_SOS_TRIGGER_URL, payload);
}

bool SOSRouting::_sendViaHTTP(const char* url, const char* payload) {
    if (WiFi.status() != WL_CONNECTED) return false;

    Serial.printf("[HTTP] POST %s\n", url);
    Serial.printf("[HTTP] Payload: %s\n", payload);

    WiFiClientSecure client;
    client.setInsecure(); // Same as existing firmware

    // Reduce TLS buffers for memory efficiency
    #if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
        client.setRxBufferSize(AWS_TLS_RX_BUFFER);
        client.setTxBufferSize(AWS_TLS_TX_BUFFER);
    #else
        client.setHandshakeTimeout(10);
    #endif

    HTTPClient http;
    http.setTimeout(AWS_HTTP_TIMEOUT_MS);

    bool success = false;

    if (http.begin(client, url)) {
        http.addHeader("Content-Type", "application/json");
        int code = http.POST((uint8_t*)payload, strlen(payload));

        if (code > 0) {
            String resp = http.getString();
            Serial.printf("[HTTP] Response %d: %s\n", code, resp.c_str());
            success = (code == 200);
        } else {
            Serial.printf("[HTTP] Error: %s\n", http.errorToString(code).c_str());
        }
        http.end();
    }

    return success;
}
