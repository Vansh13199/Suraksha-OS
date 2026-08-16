// ============================================================================
// Suraksha+ Watch OS — Vibration Driver Implementation
// ============================================================================
#include "vibration.h"

Vibration& Vibration::getInstance() {
    static Vibration instance;
    return instance;
}

void Vibration::init() {
    ledcSetup(VIBRATION_PWM_CHANNEL, VIBRATION_PWM_FREQ, VIBRATION_PWM_RES);
    ledcAttachPin(PIN_VIBRATION, VIBRATION_PWM_CHANNEL);
    ledcWrite(VIBRATION_PWM_CHANNEL, 0);

    Serial.println("[VIB] Vibration motor initialized");
}

void Vibration::play(HapticPattern pattern) {
    if (!_enabled) return;

    // Cancel any active pattern
    stop();

    switch (pattern) {
        case HapticPattern::TAP:
            pulse(50);
            break;
        case HapticPattern::DOUBLE_TAP:
            pulse(50, 80, 2);
            break;
        case HapticPattern::TRIPLE_TAP:
            pulse(50, 80, 3);
            break;
        case HapticPattern::SOS_CONFIRM:
            pulse(500);
            break;
        case HapticPattern::SOS_ACTIVE_PULSE:
            pulse(200, 300, 3);
            break;
        case HapticPattern::NOTIFICATION:
            pulse(100, 150, 2);
            break;
        case HapticPattern::ALARM:
            pulse(300, 200, 5);
            break;
        case HapticPattern::ERROR:
            pulse(80, 80, 3);
            break;
        case HapticPattern::SUCCESS:
            pulse(100, 100, 2);
            break;
        case HapticPattern::WARNING:
            pulse(400);
            break;
        case HapticPattern::BUTTON_PRESS:
            pulse(30);
            break;
        case HapticPattern::ACTIVITY_ON:
            pulse(200, 200, 4);   // Match existing: vibratePulse(4, 200, 200)
            break;
        case HapticPattern::ACTIVITY_OFF:
            pulse(200, 200, 3);   // Match existing: vibratePulse(3, 200, 200)
            break;
        case HapticPattern::IDENTITY_SYNC:
            pulse(100, 100, 2);   // Match existing: vibratePulse(2, 100, 100)
            break;
        case HapticPattern::IDENTITY_ERROR:
            pulse(100, 100, 5);   // Match existing: vibratePulse(5, 100, 100)
            break;
    }
}

void Vibration::pulse(uint16_t onMs, uint16_t offMs, uint8_t count) {
    if (!_enabled) return;

    _pattern.active = true;
    _pattern.step = 0;
    _pattern.totalSteps = count;
    _pattern.onMs = onMs;
    _pattern.offMs = offMs;
    _pattern.stepStartMs = millis();
    _pattern.isOn = true;
    _motorOn();
}

void Vibration::continuous(uint16_t durationMs) {
    pulse(durationMs, 0, 1);
}

void Vibration::stop() {
    _pattern.active = false;
    _motorOff();
}

void Vibration::setIntensity(uint8_t intensity) {
    _intensity = intensity;
    // If motor is currently on, update PWM
    if (_motorActive) {
        ledcWrite(VIBRATION_PWM_CHANNEL, _intensity);
    }
}

void Vibration::update() {
    if (!_pattern.active) return;

    uint32_t now = millis();
    uint32_t elapsed = now - _pattern.stepStartMs;

    if (_pattern.isOn) {
        // Motor is currently ON
        if (elapsed >= _pattern.onMs) {
            _motorOff();
            _pattern.isOn = false;
            _pattern.step++;
            _pattern.stepStartMs = now;

            // Check if pattern is complete
            if (_pattern.step >= _pattern.totalSteps) {
                _pattern.active = false;
                return;
            }
        }
    } else {
        // Motor is currently OFF (gap between pulses)
        if (elapsed >= _pattern.offMs) {
            _motorOn();
            _pattern.isOn = true;
            _pattern.stepStartMs = now;
        }
    }
}

void Vibration::_motorOn() {
    if (!_enabled) return;
    ledcWrite(VIBRATION_PWM_CHANNEL, _intensity);
    _motorActive = true;
}

void Vibration::_motorOff() {
    ledcWrite(VIBRATION_PWM_CHANNEL, 0);
    _motorActive = false;
}
