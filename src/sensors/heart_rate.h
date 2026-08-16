// ============================================================================
// Suraksha+ Watch OS — Heart Rate Sensor (MAX30102)
// Ported from existing final.ino with improvements
// ============================================================================
#ifndef HEART_RATE_H
#define HEART_RATE_H

#include <Arduino.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "../config/app_config.h"

struct HeartRateData {
    float bpm;              // Current average BPM
    float rawBpm;           // Latest single reading
    float spo2;             // SpO2 percentage (if available)
    bool fingerPresent;     // Is finger on sensor?
    uint8_t confidence;     // Signal quality (0-100)
    uint32_t timestamp;     // Last valid reading time
};

class HeartRateSensor {
public:
    bool init();
    void update();          // Call at sensor polling rate
    HeartRateData getData() const { return _data; }
    float getAvgBPM() const;
    bool isReading() const { return _data.fingerPresent; }

private:
    MAX30105 _sensor;
    HeartRateData _data = {};

    // Rolling BPM average buffer (matching existing firmware)
    static const int BUF_SIZE = HR_SAMPLE_WINDOW;
    float _bpmBuf[BUF_SIZE] = {};
    int _bufIdx = 0;
    int _bufCount = 0;

    unsigned long _lastBeat = 0;
    bool _initialized = false;
};

#endif // HEART_RATE_H
