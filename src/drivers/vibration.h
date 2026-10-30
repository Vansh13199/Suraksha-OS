// ============================================================================
// Suraksha+ Watch OS — Vibration / Haptic Feedback Driver
// ============================================================================
#ifndef VIBRATION_H
#define VIBRATION_H

#include <Arduino.h>
#include "../config/hardware_config.h"

// Predefined haptic patterns
enum class HapticPattern {
    TAP,                // Single short pulse (50ms)
    DOUBLE_TAP,         // Two quick pulses
    TRIPLE_TAP,         // Three quick pulses
    SOS_CONFIRM,        // Strong 2-second buzz
    SOS_STOP,           // 1-second continuous buzz
    SOS_ACTIVE_PULSE,   // Periodic pulse during active SOS
    NOTIFICATION,       // Gentle double-pulse
    ALARM,              // Escalating pattern
    ERROR,              // Rapid triple pulse
    SUCCESS,            // Two ascending pulses
    WARNING,            // Long medium pulse
    BUTTON_PRESS,       // Ultra-short feedback (30ms)
    ACTIVITY_ON,        // 4 pulses (match existing firmware)
    ACTIVITY_OFF,       // 3 pulses (match existing firmware)
    IDENTITY_SYNC,      // 2 short pulses (match existing firmware)
    IDENTITY_ERROR      // 5 rapid pulses (match existing firmware)
};

class Vibration {
public:
    static Vibration& getInstance();

    void init();

    // Play a predefined pattern (non-blocking)
    void play(HapticPattern pattern);

    // Custom vibration
    void pulse(uint16_t onMs, uint16_t offMs = 0, uint8_t count = 1);
    void continuous(uint16_t durationMs);
    void stop();

    // Intensity control
    void setIntensity(uint8_t intensity); // 0-255
    uint8_t getIntensity() const { return _intensity; }
    void setEnabled(bool enabled) { _enabled = enabled; }
    bool isEnabled() const { return _enabled; }

    // Non-blocking update (call from main loop or timer)
    void update();

private:
    Vibration() = default;

    void _motorOn();
    void _motorOff();

    uint8_t _intensity = 200;
    bool _enabled = true;
    bool _motorActive = false;
    bool _hasDRV2605 = false;

    // Pattern playback state
    struct PatternState {
        bool active;
        uint8_t step;
        uint8_t totalSteps;
        uint16_t onMs;
        uint16_t offMs;
        uint32_t stepStartMs;
        bool isOn;
    };
    PatternState _pattern = {};
};

#endif // VIBRATION_H
