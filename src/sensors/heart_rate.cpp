// ============================================================================
// Suraksha+ Watch OS — Heart Rate Sensor Implementation
// Ported from final.ino with the exact same MAX30102 configuration
// ============================================================================
#include "heart_rate.h"
#include "../config/hardware_config.h"

bool HeartRateSensor::init() {
    // Initialize MAX30102 (same config as existing firmware)
    if (!_sensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println("[HR] ❌ MAX30102 not found!");
        _initialized = false;
        return false;
    }

    // Exact same setup as final.ino:
    // sensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange)
    _sensor.setup(0xFF, 8, 2, 200, 411, 16384);
    _sensor.setPulseAmplitudeIR(0xFF);
    _sensor.setPulseAmplitudeRed(0x4F);
    _sensor.setPulseAmplitudeGreen(0);

    _initialized = true;
    Serial.println("[HR] ✅ MAX30102 initialized");
    return true;
}

void HeartRateSensor::update() {
    if (!_initialized) return;

    _sensor.check();

    while (_sensor.available()) {
        long ir = _sensor.getIR();
        _sensor.nextSample();

        // Check if finger is on sensor (same threshold as existing firmware)
        if (ir < HR_IR_THRESHOLD) {
            _data.fingerPresent = false;
            _bufCount = 0;
            _bufIdx = 0;
            continue;
        }

        _data.fingerPresent = true;

        // Check for heartbeat (uses SparkFun library's built-in algorithm)
        if (checkForBeat(ir)) {
            unsigned long now = millis();
            unsigned long dt = now - _lastBeat;
            _lastBeat = now;

            // Validate beat interval (same range as existing firmware)
            if (dt > HR_BEAT_MIN_INTERVAL_MS && dt < HR_BEAT_MAX_INTERVAL_MS) {
                float bpm = 60.0f / (dt / 1000.0f);

                // Validate BPM range
                if (bpm >= HR_MIN_VALID_BPM && bpm <= HR_MAX_VALID_BPM) {
                    _data.rawBpm = bpm;

                    // Add to rolling average buffer
                    _bpmBuf[_bufIdx] = bpm;
                    _bufIdx = (_bufIdx + 1) % BUF_SIZE;
                    if (_bufCount < BUF_SIZE) _bufCount++;

                    _data.bpm = getAvgBPM();
                    _data.timestamp = now;
                    _data.confidence = min(100, (int)(_bufCount * 100 / BUF_SIZE));
                }
            }
        }
    }
}

float HeartRateSensor::getAvgBPM() const {
    if (_bufCount == 0) return 0.0f;
    float sum = 0;
    for (int i = 0; i < _bufCount; i++) {
        sum += _bpmBuf[i];
    }
    return sum / _bufCount;
}
