// ============================================================================
// Suraksha+ Watch OS — SOS Manager Implementation
// ============================================================================
#include "sos_manager.h"
#include <esp_random.h>

void SOSManager::init() {
    _active = false;
    memset(_sessionId, 0, sizeof(_sessionId));
    _lastTriggerMs = 0;
    Serial.println("[SOS] SOS manager initialized");
}

void SOSManager::trigger(const char* reason) {
    // Check cooldown
    if (_lastTriggerMs > 0 && (millis() - _lastTriggerMs < SOS_COOLDOWN_MS)) {
        Serial.println("[SOS] Cooldown active — ignoring trigger");
        return;
    }

    // Generate fresh session ID
    _generateUUID(_sessionId);

    _active = true;
    _reason = reason;
    _startMs = millis();
    _lastTriggerMs = millis();
    _lastUpdateMs = millis();
    _updateCount = 0;

    Serial.println("\n🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨");
    Serial.println("     EMERGENCY SOS TRIGGERED");
    Serial.printf("  Reason: %s\n", reason);
    Serial.printf("  Session: %s\n", _sessionId);
    Serial.println("🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨\n");
}

void SOSManager::stop() {
    if (!_active) return;

    Serial.printf("[SOS] Session ended: %s (Duration: %ds)\n",
                  _sessionId, getDurationMs() / 1000);

    _active = false;
    _reason = "";
    // Cooldown starts NOW so no new session can immediately start
    _lastTriggerMs = millis();
}

void SOSManager::update() {
    if (!_active) return;

    // Check if it's time for a live update
    if (millis() - _lastUpdateMs >= SOS_LIVE_UPDATE_INTERVAL_MS) {
        _lastUpdateMs = millis();
        _updateCount++;

        Serial.printf("[SOS] Live update #%d (Session: %s, Duration: %ds)\n",
                      _updateCount, _sessionId, getDurationMs() / 1000);
    }
}

uint32_t SOSManager::getDurationMs() const {
    if (!_active) return 0;
    return millis() - _startMs;
}

void SOSManager::_generateUUID(char* out) {
    const char hex[] = "0123456789abcdef";
    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            out[i] = '-';
        } else if (i == 14) {
            out[i] = '4';
        } else if (i == 19) {
            out[i] = hex[8 + (esp_random() % 4)];
        } else {
            out[i] = hex[esp_random() % 16];
        }
    }
    out[36] = '\0';
}
