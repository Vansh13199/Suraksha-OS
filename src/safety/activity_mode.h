// ============================================================================
// Suraksha+ Watch OS — Activity Mode
// ============================================================================
#ifndef ACTIVITY_MODE_H
#define ACTIVITY_MODE_H

#include <Arduino.h>
#include "../config/app_config.h"

class ActivityMode {
public:
    void init();
    void start(uint32_t durationSeconds);
    void stop();
    void update();

    bool isActive() const { return _active; }
    uint32_t getRemainingSeconds() const;
    uint32_t getTotalDurationSeconds() const { return _durationSeconds; }
    uint32_t getElapsedSeconds() const;

private:
    bool _active = false;
    uint32_t _durationSeconds = 0;
    uint32_t _startMs = 0;
    uint32_t _endMs = 0;
};

#endif // ACTIVITY_MODE_H
