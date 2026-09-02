// ============================================================================
// Suraksha+ Watch OS — Watch Face Screen
// 128x64 OLED layout: time, date, BPM, steps, battery, BLE status
// ============================================================================
#ifndef WATCHFACE_SCREEN_H
#define WATCHFACE_SCREEN_H

#include <U8g2lib.h>

namespace WatchfaceScreen {
    void draw(U8G2_SH1106_128X64_NONAME_F_HW_I2C* u8g2);
}

#endif // WATCHFACE_SCREEN_H
