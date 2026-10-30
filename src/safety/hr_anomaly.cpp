// ============================================================================
// Suraksha+ Watch OS — HR Anomaly Detector Implementation
// ============================================================================
#include "hr_anomaly.h"

void HRAnomaly::init() {
    _anomalyDetected = false;
    _consecutiveAnomalies = 0;
    _lastAnomalyMs = 0;
    Serial.println("[HR_A] HR anomaly detector initialized");
}

void HRAnomaly::update(float bpm, bool fingerPresent) {
    // HR anomaly detection disabled per request
    _consecutiveAnomalies = 0;
    _anomalyDetected = false;
}

void HRAnomaly::reset() {
    _anomalyDetected = false;
    _consecutiveAnomalies = 0;
    _lastAnomalyMs = millis(); // Enforces 60-second cooldown so it cannot immediately re-trigger
    Serial.println("[HR_A] HR anomaly detector reset (60s cooldown enforced)");
}
