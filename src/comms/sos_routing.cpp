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

SOSRoute SOSRouting::getBestRoute() const {
    // Priority 1: BLE → Phone (phone has internet + GPS)
    if (BleService::getInstance().isConnected()) {
        return SOSRoute::BLE_TO_PHONE;
    }

    // Priority 2: Direct WiFi → AWS
    if (WiFi.status() == WL_CONNECTED) {
        return SOSRoute::WIFI_DIRECT;
    }

    // Priority 3: Store and forward
    return SOSRoute::STORE_AND_FORWARD;
}

bool SOSRouting::sendSOS(const char* phoneNumber, const char* sessionId,
                         const char* lat, const char* lon, float heartRate) {
    SOSRoute route = getBestRoute();

    Serial.printf("[ROUTE] SOS via %s\n",
                  route == SOSRoute::BLE_TO_PHONE ? "BLE→Phone" :
                  route == SOSRoute::WIFI_DIRECT ? "WiFi→AWS" : "Store&Forward");

    switch (route) {
        case SOSRoute::BLE_TO_PHONE:
            return _sendViaBLE(sessionId, lat, lon);

        case SOSRoute::WIFI_DIRECT:
            return _sendViaWiFi(phoneNumber, sessionId, lat, lon, heartRate);

        case SOSRoute::STORE_AND_FORWARD:
            // TODO: Store in NVS queue for later transmission
            Serial.println("[ROUTE] ⚠️ No connection — SOS queued");
            return false;

        default:
            return false;
    }
}

bool SOSRouting::stopSOS(const char* sessionId) {
    SOSRoute route = getBestRoute();

    if (route == SOSRoute::BLE_TO_PHONE) {
        BleService::getInstance().send(BLE_MSG_SOS_STOP);
        return true;
    }

    if (route == SOSRoute::WIFI_DIRECT) {
        char payload[128];
        snprintf(payload, sizeof(payload), "{\"sessionId\":\"%s\"}", sessionId);
        return _sendViaHTTP(AWS_SOS_STOP_URL, payload);
    }

    return false;
}

bool SOSRouting::sendLiveUpdate(const char* phoneNumber, const char* sessionId,
                                const char* lat, const char* lon, float heartRate) {
    SOSRoute route = getBestRoute();

    switch (route) {
        case SOSRoute::BLE_TO_PHONE:
            // Send location update via BLE
            BleService::getInstance().sendFormatted("SOS:UPDATE:%s,%s,%.1f",
                                                     lat, lon, heartRate);
            return true;

        case SOSRoute::WIFI_DIRECT:
            return _sendViaWiFi(phoneNumber, sessionId, lat, lon, heartRate);

        default:
            return false;
    }
}

bool SOSRouting::_sendViaBLE(const char* sessionId, const char* lat, const char* lon) {
    BleService& ble = BleService::getInstance();

    // Send SOS:START to phone — phone will handle API call
    ble.send(BLE_MSG_SOS_START);

    // Also send location data
    ble.sendFormatted("SOS:LOC:%s,%s", lat, lon);

    return true;
}

bool SOSRouting::_sendViaWiFi(const char* phoneNumber, const char* sessionId,
                              const char* lat, const char* lon, float heartRate) {
    // Same HTTPS call as existing firmware
    char payload[320];
    snprintf(payload, sizeof(payload),
        "{\"phoneNumber\":\"%s\",\"sessionId\":\"%s\",\"latitude\":%s,\"longitude\":%s,\"heartRate\":%.1f}",
        phoneNumber, sessionId, lat, lon, heartRate);

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
