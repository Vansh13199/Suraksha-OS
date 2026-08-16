// ============================================================================
// Suraksha+ Watch OS — Buzzer Driver Implementation
// ============================================================================
#include "buzzer.h"

Buzzer& Buzzer::getInstance() {
    static Buzzer instance;
    return instance;
}

void Buzzer::init() {
#if BUZZER_ENABLED
    if (PIN_BUZZER >= 0) {
        ledcSetup(BUZZER_PWM_CHANNEL, 2000, 8);
        ledcAttachPin(PIN_BUZZER, BUZZER_PWM_CHANNEL);
        ledcWrite(BUZZER_PWM_CHANNEL, 0);
        _available = true;
        Serial.println("[BUZ] Buzzer initialized");
    }
#else
    _available = false;
    Serial.println("[BUZ] Buzzer not configured");
#endif
}

void Buzzer::tone(uint16_t frequency, uint16_t durationMs) {
#if BUZZER_ENABLED
    if (!_available) return;
    ledcWriteTone(BUZZER_PWM_CHANNEL, frequency);
    // Note: Caller should handle timing for non-blocking operation
#endif
}

void Buzzer::alarm() {
#if BUZZER_ENABLED
    if (!_available) return;
    // Play a brief alarm tone
    ledcWriteTone(BUZZER_PWM_CHANNEL, 2000);
#endif
}

void Buzzer::stop() {
#if BUZZER_ENABLED
    if (!_available) return;
    ledcWriteTone(BUZZER_PWM_CHANNEL, 0);
#endif
}
