// ============================================================================
// Suraksha+ Watch OS — Heart Rate Anomaly Detector
// ============================================================================
#ifndef HR_ANOMALY_H
#define HR_ANOMALY_H

#include <Arduino.h>
#include "../config/app_config.h"

class HRAnomaly {
public:
    void init();
    void update(float bpm, bool fingerPresent);
    void reset();

    bool isAnomalyDetected() const { return _anomalyDetected; }

private:
    bool _anomalyDetected = false;
    int _consecutiveAnomalies = 0;
    uint32_t _lastAnomalyMs = 0;
};

#endif // HR_ANOMALY_H
