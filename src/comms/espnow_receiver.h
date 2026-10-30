// ============================================================================
// Suraksha+ Watch OS — ESP-NOW Receiver
// Receives heart rate data from the Seeed XIAO ESP32S3 sensor module
// ============================================================================
#ifndef ESPNOW_RECEIVER_H
#define ESPNOW_RECEIVER_H

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// ============================================================================
// ESP-NOW Heart Rate Packet (MUST match sender's definition exactly)
// ============================================================================
typedef struct __attribute__((packed)) {
    uint8_t  packetType;       // 0x01 = heart rate data, 0x02 = ping
    float    bpm;              // Smoothed BPM
    float    rawBpm;           // Raw instantaneous BPM
    float    spo2;             // SpO2 percentage
    uint8_t  confidence;       // Signal quality 0-100
    bool     fingerPresent;    // Is finger on sensor?
    uint32_t timestamp;        // millis() on sender
    uint8_t  batteryPercent;   // Sender battery level
} HRPacket_t;

#define PACKET_TYPE_HEARTRATE  0x01
#define PACKET_TYPE_PING       0x02

// ============================================================================
// Received data (thread-safe access via mutex)
// ============================================================================
struct ESPNowHRData {
    float    bpm;
    float    rawBpm;
    float    spo2;
    uint8_t  confidence;
    bool     fingerPresent;
    uint32_t lastReceived;     // Local millis() when last packet arrived
    uint8_t  senderBattery;
    bool     senderOnline;     // True if packets received recently
    int8_t   rssi;             // Signal strength of last packet
    uint32_t packetsReceived;  // Total packet count
    uint32_t packetsLost;      // Estimated lost packets
};

// Timeout: if no packet received for this long, sender is considered offline
#define ESPNOW_TIMEOUT_MS        10000
#define ESPNOW_WIFI_CHANNEL      1

class ESPNowReceiver {
public:
    static ESPNowReceiver& getInstance();

    // Initialize ESP-NOW receiver. Call BEFORE WiFi is fully configured.
    // ESP-NOW can coexist with WiFi STA mode but the channel must match.
    bool init();

    // Check if the sender is online (received data within timeout)
    bool isSenderOnline() const;

    // Get a thread-safe copy of the latest received HR data
    ESPNowHRData getData() const;

    // Get just the BPM value (convenience)
    float getBPM() const;
    float getSpO2() const;
    bool  isFingerPresent() const;

    // Get signal quality
    int8_t getRSSI() const;
    uint32_t getPacketsReceived() const;

private:
    ESPNowReceiver() = default;

#include <esp_idf_version.h>

    // Static callback required by ESP-NOW API
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    static void onDataRecvCallback(const esp_now_recv_info_t *info,
                                   const uint8_t *data, int data_len);
#else
    static void onDataRecvCallback(const uint8_t *mac_addr,
                                   const uint8_t *data, int data_len);
#endif

    // Instance method to process received data
    void handleReceivedData(const uint8_t *mac, const uint8_t *data,
                            int len, int8_t rssi);

    ESPNowHRData _data = {};
    SemaphoreHandle_t _mutex = nullptr;
    bool _initialized = false;

    // For packet loss estimation
    uint32_t _lastSenderTimestamp = 0;
};

#endif // ESPNOW_RECEIVER_H
