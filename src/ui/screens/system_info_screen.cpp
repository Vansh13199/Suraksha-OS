// ============================================================================
// Suraksha+ Watch OS — System Info Screen Implementation
// 128x64 OLED layout using U8g2
// ============================================================================
#include "system_info_screen.h"
#include "../../comms/ble_service.h"
#include "../../sensors/sensor_manager.h"
#include "../../config/app_config.h"
#include <esp_system.h>

namespace SystemInfoScreen {

    void draw(U8G2_SH1106_128X64_NONAME_F_HW_I2C* u8g2) {
        // --- Title ---
        u8g2->setFont(u8g2_font_6x10_tr);
        u8g2->drawStr(0, 9, "SYSTEM INFO");
        u8g2->drawHLine(0, 11, 128);

        u8g2->setFont(u8g2_font_5x7_tr);

        // --- Heap / PSRAM ---
        char heapBuf[28];
        snprintf(heapBuf, sizeof(heapBuf), "Heap:%dKB PSRAM:%dKB",
                 ESP.getFreeHeap() / 1024,
                 ESP.getFreePsram() / 1024);
        u8g2->drawStr(0, 22, heapBuf);

        // --- CPU / Temp ---
        char cpuBuf[28];
        snprintf(cpuBuf, sizeof(cpuBuf), "CPU:%dMHz Temp:%.0fC",
                 ESP.getCpuFreqMHz(),
                 temperatureRead());
        u8g2->drawStr(0, 32, cpuBuf);

        // --- BLE status ---
        bool bleConn = BleService::getInstance().isConnected();
        u8g2->drawStr(0, 42, bleConn ? "BLE: Connected" : "BLE: Disconnected");

        // --- Uptime ---
        uint32_t uptimeSec = millis() / 1000;
        uint32_t uptimeMin = uptimeSec / 60;
        uptimeSec %= 60;
        uint32_t uptimeHr = uptimeMin / 60;
        uptimeMin %= 60;

        char uptimeBuf[24];
        if (uptimeHr > 0) {
            snprintf(uptimeBuf, sizeof(uptimeBuf), "Up: %dh %dm %ds",
                     uptimeHr, uptimeMin, uptimeSec);
        } else {
            snprintf(uptimeBuf, sizeof(uptimeBuf), "Up: %dm %ds",
                     uptimeMin, uptimeSec);
        }
        u8g2->drawStr(0, 52, uptimeBuf);

        // --- Version ---
        char verBuf[28];
        snprintf(verBuf, sizeof(verBuf), "Suraksha+ v%s", WATCH_OS_VERSION);
        u8g2->drawStr(0, 63, verBuf);
    }

} // namespace SystemInfoScreen
