// ============================================================================
// Suraksha+ Watch OS — Health Screen
// 240x240 TFT layout: remote HR, SpO2, steps, motion score, sensor health
// ============================================================================
#ifndef HEALTH_SCREEN_H
#define HEALTH_SCREEN_H

#include <TFT_eSPI.h>

namespace HealthScreen {
    void draw(TFT_eSPI* tft);
}

#endif // HEALTH_SCREEN_H
