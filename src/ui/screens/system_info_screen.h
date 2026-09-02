// ============================================================================
// Suraksha+ Watch OS — System Info Screen
// 128x64 OLED layout: heap, PSRAM, CPU, BLE, uptime, version
// ============================================================================
#ifndef SYSTEM_INFO_SCREEN_H
#define SYSTEM_INFO_SCREEN_H

#include <U8g2lib.h>

namespace SystemInfoScreen {
    void draw(U8G2_SH1106_128X64_NONAME_F_HW_I2C* u8g2);
}

#endif // SYSTEM_INFO_SCREEN_H
