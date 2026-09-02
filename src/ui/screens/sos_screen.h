// ============================================================================
// Suraksha+ Watch OS — SOS Screen
// 128x64 OLED layout: SOS alert with timer, GPS, BPM
// ============================================================================
#ifndef SOS_SCREEN_H
#define SOS_SCREEN_H

#include <U8g2lib.h>

namespace SOSScreen {
    void draw(U8G2_SH1106_128X64_NONAME_F_HW_I2C* u8g2);
}

#endif // SOS_SCREEN_H
