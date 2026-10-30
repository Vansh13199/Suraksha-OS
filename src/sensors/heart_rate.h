// ============================================================================
// Suraksha+ Watch OS — Heart Rate Sensor (ESP-NOW Remote)
// Receives HR data from external Seeed XIAO ESP32S3 via ESP-NOW
// ============================================================================
#ifndef HEART_RATE_H
#define HEART_RATE_H

#include <Arduino.h>
#include "../config/app_config.h"

struct HeartRateData {
    float bpm;              // Current smoothed BPM (from remote sensor)
    float rawBpm;           // Latest raw reading from remote sensor
    float spo2;             // SpO2 percentage
    bool fingerPresent;     // Is finger on remote sensor?
    uint8_t confidence;     // Signal quality (0-100)
    uint32_t timestamp;     // Last valid reading time (local millis)
    bool sensorOnline;      // Is the remote sensor module reachable?
    int8_t rssi;            // ESP-NOW signal strength
    uint8_t sensorBattery;  // Remote sensor battery level
};

class HeartRateSensor {
public:
    bool init();
    void update();          // Pulls latest data from ESPNowReceiver
    HeartRateData getData() const { return _data; }
    float getAvgBPM() const;
    bool isReading() const { return _data.fingerPresent && _data.sensorOnline; }
    bool isSensorOnline() const { return _data.sensorOnline; }

private:
    HeartRateData _data = {};

    // EMA smoothing for BPM (applied on top of remote sensor's averaging)
    float _smoothedBpm = 0;
    float _lastBestBpm = 0;
    bool _firstReading = true;

    bool _initialized = false;
};

#endif // HEART_RATE_H
