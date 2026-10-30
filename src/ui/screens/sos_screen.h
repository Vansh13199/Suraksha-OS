// ============================================================================
// Suraksha+ Watch OS — SOS Screen
// 240x240 TFT layout: emergency alert, timer, GPS, HR, status
// ============================================================================
#ifndef SOS_SCREEN_H
#define SOS_SCREEN_H

#include <TFT_eSPI.h>

namespace SOSScreen {
    void draw(TFT_eSPI* tft);
}

#endif // SOS_SCREEN_H
