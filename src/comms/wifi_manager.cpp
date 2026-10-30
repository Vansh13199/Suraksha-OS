// ============================================================================
// Suraksha+ Watch OS — WiFi & NTP Manager Implementation
// ============================================================================
#include "wifi_manager.h"
#include <WiFi.h>
#include <time.h>
#include "../config/app_config.h"

WifiManager& WifiManager::getInstance() {
    static WifiManager instance;
    return instance;
}

#include "espnow_receiver.h"
#include <esp_wifi.h>

bool WifiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool WifiManager::connect(uint32_t timeoutMs) {
    if (String(WIFI_SSID) == "YOUR_WIFI_SSID" || strlen(WIFI_SSID) == 0) {
        Serial.println("[WIFI] SSID not configured. Skipping WiFi connection.");
        return false;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[WIFI] Already connected to WiFi AP.");
        return true;
    }

    Serial.printf("[WIFI] Connecting to '%s' (timeout: %dms)...\n", WIFI_SSID, timeoutMs);
    
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    for (int attempt = 1; attempt <= 2; attempt++) {
        Serial.printf("[WIFI] Attempt %d/2: begin('%s')...\n", attempt, WIFI_SSID);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        uint32_t startMs = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startMs < (timeoutMs / 2))) {
            delay(250);
            Serial.print(".");
            if (WiFi.status() == WL_CONNECT_FAILED) {
                Serial.println(" [Auth/Password Failed]");
                break;
            }
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("[WIFI] ✅ Connected to WiFi AP!");
            Serial.printf("[WIFI] IP: %s | RSSI: %d dBm | Channel: %d\n",
                          WiFi.localIP().toString().c_str(), WiFi.RSSI(), WiFi.channel());
            delay(200); // Allow network stack to settle
            return true;
        }

        delay(500);
    }

    int st = WiFi.status();
    const char* statusDesc = "Unknown";
    switch (st) {
        case WL_NO_SSID_AVAIL: statusDesc = "SSID not found (Check 2.4GHz / Hotspot ON)"; break;
        case WL_CONNECT_FAILED: statusDesc = "Connection failed (Check Password)"; break;
        case WL_CONNECTION_LOST: statusDesc = "Connection lost"; break;
        case WL_DISCONNECTED: statusDesc = "Disconnected / Handshake Timeout"; break;
        case WL_IDLE_STATUS: statusDesc = "Idle / Not Started"; break;
    }
    Serial.printf("[WIFI] ⚠️ Failed to connect to '%s'. Status (%d): %s\n", WIFI_SSID, st, statusDesc);

    // Quick 2.4GHz scan to verify if SSID is visible in the air
    Serial.println("[WIFI] Scanning nearby 2.4GHz networks for diagnosis...");
    _lastScanFoundSSID = false;
    int n = WiFi.scanNetworks(false, false, false, 200);
    for (int i = 0; i < n; i++) {
        if (WiFi.SSID(i) == WIFI_SSID) {
            _lastScanFoundSSID = true;
            Serial.printf("[WIFI] 🔍 Found '%s' in 2.4GHz scan (RSSI: %ddBm, Ch: %d) -> check password or DHCP\n",
                          WIFI_SSID, WiFi.RSSI(i), WiFi.channel(i));
            break;
        }
    }
    if (!_lastScanFoundSSID) {
        Serial.printf("[WIFI] ❌ '%s' was NOT found in 2.4GHz scan. The hotspot is either OFF or 5GHz-only!\n", WIFI_SSID);
    }
    
    // In failure case, make sure auto-reconnect is stopped and ESP-NOW channel is restored
    WiFi.disconnect(false);
    WiFi.setAutoReconnect(false);
    esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
    return false;
}

void WifiManager::disconnect() {
    WiFi.disconnect(false); // Disconnect from AP but keep STA radio active for ESP-NOW
    WiFi.setAutoReconnect(false);
    // Restore ESP-NOW WiFi channel so HR sensor packets continue to be received
    esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
    Serial.printf("[WIFI] Disconnected from AP. STA radio active on channel %d for ESP-NOW.\n",
                  ESPNOW_WIFI_CHANNEL);
}

#include <esp_sntp.h>

static volatile bool s_ntpPacketReceived = false;

static void ntpSyncCallback(struct timeval *tv) {
    s_ntpPacketReceived = true;
    Serial.println("[NTP] ✅ Live NTP packet received and applied to system clock!");
}

bool WifiManager::syncTime() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[NTP] ❌ Cannot sync time: WiFi not connected.");
        setenv("TZ", "IST-5:30", 1);
        tzset();
        return false;
    }

    Serial.printf("[NTP] Requesting live IST time (+5:30) from %s, time.google.com, time.cloudflare.com...\n", NTP_SERVER);
    
    s_ntpPacketReceived = false;
    sntp_set_time_sync_notification_cb(ntpSyncCallback);

    // configTzTime configures the POSIX timezone "IST-5:30" (UTC+5:30) and NTP servers
    configTzTime("IST-5:30", "time.google.com", NTP_SERVER, "time.cloudflare.com");

    // Wait until an authentic NTP packet is actually received and applied
    uint32_t startMs = millis();
    while (!s_ntpPacketReceived && (millis() - startMs < 10000)) {
        delay(200);
        Serial.print(".");
        // Also check SNTP sync status
        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
            s_ntpPacketReceived = true;
            break;
        }
    }
    Serial.println();

    if (s_ntpPacketReceived) {
        setenv("TZ", "IST-5:30", 1);
        tzset();
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 500)) {
            char timeStr[64];
            strftime(timeStr, sizeof(timeStr), "%A, %d %b %Y %I:%M:%S %p", &timeinfo);
            Serial.printf("[NTP] ✅ Confirmed real-time synchronized: %s (IST)\n", timeStr);
        }
        return true;
    } else {
        Serial.println("[NTP] ⚠️ NTP server response timed out. Keeping hardware clock.");
        return false;
    }
}
