// ============================================================================
// Suraksha+ Watch OS — Health Screen
// 128x64 OLED layout: heart rate, steps, motion, GPS
// ============================================================================
#ifndef HEALTH_SCREEN_H
#define HEALTH_SCREEN_H

#include <U8g2lib.h>

namespace HealthScreen {
    void draw(U8G2_SH1106_128X64_NONAME_F_HW_I2C* u8g2);
}

#endif // HEALTH_SCREEN_H
