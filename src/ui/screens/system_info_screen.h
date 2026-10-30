// ============================================================================
// Suraksha+ Watch OS — System Info Screen
// 240x240 TFT layout: hardware stats, memory, ESP-NOW link, BLE, uptime
// ============================================================================
#ifndef SYSTEM_INFO_SCREEN_H
#define SYSTEM_INFO_SCREEN_H

#include <TFT_eSPI.h>

namespace SystemInfoScreen {
    void draw(TFT_eSPI* tft);
}

#endif // SYSTEM_INFO_SCREEN_H
