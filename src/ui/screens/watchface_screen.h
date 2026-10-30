// ============================================================================
// Suraksha+ Watch OS — Watch Face Screen
// 240x240 TFT layout: time, date, BPM, steps, battery, BLE, ESP-NOW link
// ============================================================================
#ifndef WATCHFACE_SCREEN_H
#define WATCHFACE_SCREEN_H

#include <TFT_eSPI.h>

namespace WatchfaceScreen {
    void draw(TFT_eSPI* tft);
}

#endif // WATCHFACE_SCREEN_H
