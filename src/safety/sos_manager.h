// ============================================================================
// Suraksha+ Watch OS — SOS Manager
// SOS lifecycle management and session tracking
// ============================================================================
#ifndef SOS_MANAGER_H
#define SOS_MANAGER_H

#include <Arduino.h>
#include "../config/app_config.h"

class SOSManager {
public:
    void init();
    void trigger(const char* reason);
    void stop();
    void update();  // Called during active SOS for live updates

    bool isActive() const { return _active; }
    const char* getSessionId() const { return _sessionId; }
    const char* getReason() const { return _reason; }
    uint32_t getDurationMs() const;
    uint32_t getUpdateCount() const { return _updateCount; }

private:
    void _generateUUID(char* out);

    bool _active = false;
    char _sessionId[40] = "";
    const char* _reason = "";
    uint32_t _startMs = 0;
    uint32_t _lastUpdateMs = 0;
    uint32_t _lastTriggerMs = 0;
    uint32_t _updateCount = 0;
};

#endif // SOS_MANAGER_H
