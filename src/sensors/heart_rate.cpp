// ============================================================================
// Suraksha+ Watch OS — Heart Rate Sensor Implementation (ESP-NOW Remote)
// Pulls HR data from the ESP-NOW receiver (data sent by Seeed XIAO ESP32S3)
// ============================================================================
#include "heart_rate.h"
#include "../config/hardware_config.h"
#include "../comms/espnow_receiver.h"

bool HeartRateSensor::init() {
    Serial.println("[HR] Initializing remote heart rate sensor...");
    Serial.println("[HR] Data source: Seeed XIAO ESP32S3 via ESP-NOW");

    // The ESP-NOW receiver is initialized separately in main.cpp
    // We just need to verify it's available
    _initialized = true;
    _firstReading = true;
    _smoothedBpm = 0;

    Serial.println("[HR] ✅ Remote HR sensor interface ready");
    Serial.println("[HR] Waiting for ESP-NOW packets from sensor module...");
    return true;
}

void HeartRateSensor::update() {
    if (!_initialized) return;

    // Get latest data from ESP-NOW receiver
    ESPNowReceiver& receiver = ESPNowReceiver::getInstance();
    ESPNowHRData espnowData = receiver.getData();

    // Map ESP-NOW data to our HeartRateData struct
    _data.sensorOnline  = espnowData.senderOnline;
    _data.rssi          = espnowData.rssi;
    _data.sensorBattery = espnowData.senderBattery;

    if (!espnowData.senderOnline) {
        // Sensor module is offline
        _data.fingerPresent = false;
        _data.rawBpm = 0;
        _data.spo2 = 0;
        _data.confidence = 0;
        _firstReading = true;
        // Maintain last best reading so UI does not abruptly clear
        _data.bpm = _lastBestBpm;
        return;
    }

    _data.fingerPresent = espnowData.fingerPresent;
    _data.rawBpm        = espnowData.rawBpm;
    _data.spo2          = 0; // SpO2 removed
    _data.confidence    = espnowData.confidence;
    _data.timestamp     = espnowData.lastReceived;

    if (!espnowData.fingerPresent) {
        // Finger removed: reset live reading and clear false high reading
        _firstReading = true;
        _data.rawBpm = 0;
        if (_lastBestBpm > HR_ANOMALY_HIGH_BPM) {
            _lastBestBpm = 0;
            _smoothedBpm = 0;
        }
        _data.bpm = _lastBestBpm;
        return;
    }

    if (espnowData.bpm >= 40.0f && espnowData.bpm <= 220.0f) {
        // Direct real-time update: exact BPM at the exact same time
        _smoothedBpm = espnowData.bpm;
        _lastBestBpm = espnowData.bpm;
        _firstReading = false;
    }

    // Always maintain last best reading
    _data.bpm = _lastBestBpm;
}

float HeartRateSensor::getAvgBPM() const {
    return _data.bpm;
}
