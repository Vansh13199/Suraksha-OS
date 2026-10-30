// ============================================================================
// Suraksha+ Watch OS — Activity Mode Implementation
// ============================================================================
#include "activity_mode.h"

void ActivityMode::init() {
    _active = false;
    _durationSeconds = 0;
    _defaultDurationSeconds = ACTIVITY_DEFAULT_DURATION_S;
    Serial.println("[ACT] Activity mode initialized");
}

void ActivityMode::setDuration(uint32_t durationSeconds) {
    if (durationSeconds > ACTIVITY_MAX_DURATION_S) {
        durationSeconds = ACTIVITY_MAX_DURATION_S;
    }
    if (durationSeconds == 0) {
        durationSeconds = 60; // Minimum 1 minute
    }

    _defaultDurationSeconds = durationSeconds;
    _durationSeconds = durationSeconds;

    if (_active) {
        // Dynamically update the active countdown timer
        _endMs = millis() + (durationSeconds * 1000UL);
        Serial.printf("[ACT] ⏱ Active countdown timer updated via BLE to %u seconds\n", durationSeconds);
    } else {
        Serial.printf("[ACT] ⏱ Default activity timer updated via BLE to %u seconds\n", durationSeconds);
    }
}

void ActivityMode::start(uint32_t durationSeconds) {
    if (durationSeconds == 0) {
        durationSeconds = _defaultDurationSeconds;
    }
    if (durationSeconds > ACTIVITY_MAX_DURATION_S) {
        durationSeconds = ACTIVITY_MAX_DURATION_S;
    }

    _durationSeconds = durationSeconds;
    _startMs = millis();
    _endMs = _startMs + (durationSeconds * 1000UL);
    _active = true;

    Serial.printf("[ACT] ▶ Activity mode ON for %u seconds\n", durationSeconds);
}

void ActivityMode::stop() {
    if (!_active) return;
    _active = false;
    _durationSeconds = 0;
    Serial.println("[ACT] ■ Activity mode OFF");
}

void ActivityMode::update() {
    if (!_active) return;

    // Check timer expiry
    if (_endMs > 0 && millis() >= _endMs) {
        Serial.println("[ACT] ⏱ Activity mode expired");
        _active = false;
        _durationSeconds = 0;
    }
}

uint32_t ActivityMode::getRemainingSeconds() const {
    if (!_active || _endMs == 0) return 0;
    if (millis() >= _endMs) return 0;
    return (_endMs - millis()) / 1000;
}

uint32_t ActivityMode::getElapsedSeconds() const {
    if (!_active) return 0;
    return (millis() - _startMs) / 1000;
}
