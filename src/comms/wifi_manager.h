// ============================================================================
// Suraksha+ Watch OS — WiFi & NTP Manager
// Handles WiFi connections and Time Synchronization
// ============================================================================
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

class WifiManager {
public:
    static WifiManager& getInstance();

    // Connects to the configured WiFi network with optional timeout (default: 10s)
    bool connect(uint32_t timeoutMs = 10000);

    // Disconnects from AP while preserving STA radio & ESP-NOW channel
    void disconnect();

    // Syncs the ESP32 RTC time with the configured NTP server
    // Must be connected to WiFi first
    bool syncTime();

    // Checks if WiFi is currently connected to an AP
    bool isConnected();

    // Diagnostics: was the configured SSID seen during network scan?
    bool wasSSIDFound() const { return _lastScanFoundSSID; }

private:
    WifiManager() = default;
    bool _lastScanFoundSSID = false;
};

#endif // WIFI_MANAGER_H
