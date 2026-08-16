// ============================================================================
// Suraksha+ Watch OS — Buzzer Driver (Optional)
// ============================================================================
#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>
#include "../config/hardware_config.h"

class Buzzer {
public:
    static Buzzer& getInstance();

    void init();
    void tone(uint16_t frequency, uint16_t durationMs);
    void alarm();
    void stop();
    bool isAvailable() const { return _available; }

private:
    Buzzer() = default;
    bool _available = false;
};

#endif // BUZZER_H
