// ============================================================================
// Suraksha+ Watch OS — HR Anomaly Detector Implementation
// ============================================================================
#include "hr_anomaly.h"

void HRAnomaly::init() {
    _anomalyDetected = false;
    _consecutiveAnomalies = 0;
    Serial.println("[HR_A] HR anomaly detector initialized");
}

void HRAnomaly::update(float bpm, bool fingerPresent) {
    if (!fingerPresent || bpm <= 0) {
        _consecutiveAnomalies = 0;
        return;
    }

    // Check cooldown
    if (millis() - _lastAnomalyMs < SOS_AUTO_COOLDOWN_MS) {
        return;
    }

    // Check for anomalous BPM (same thresholds as existing firmware)
    bool isAnomalous = (bpm < HR_ANOMALY_LOW_BPM || bpm > HR_ANOMALY_HIGH_BPM);

    if (isAnomalous) {
        _consecutiveAnomalies++;
        Serial.printf("[HR_A] Anomalous BPM: %.1f (%d/%d consecutive)\n",
                      bpm, _consecutiveAnomalies, HR_ANOMALY_CONFIRM_COUNT);

        // Require sustained anomaly to avoid false positives
        if (_consecutiveAnomalies >= HR_ANOMALY_CONFIRM_COUNT) {
            _anomalyDetected = true;
            _lastAnomalyMs = millis();
            Serial.printf("[HR_A] 🚨 HR ANOMALY CONFIRMED: %.1f BPM\n", bpm);
        }
    } else {
        _consecutiveAnomalies = 0;
    }
}

void HRAnomaly::reset() {
    _anomalyDetected = false;
    _consecutiveAnomalies = 0;
    Serial.println("[HR_A] HR anomaly detector reset");
}
