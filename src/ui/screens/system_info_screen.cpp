// ============================================================================
// Suraksha+ Watch OS — System Info Screen Implementation
// 240x240 Color TFT layout for LilyGo T-Watch S3
// ============================================================================
#include "system_info_screen.h"
#include "../../comms/ble_service.h"
#include "../../sensors/sensor_manager.h"
#include "../../comms/espnow_receiver.h"
#include "../../config/app_config.h"
#include <esp_system.h>

namespace SystemInfoScreen {

    void draw(TFT_eSPI* tft) {
        if (!tft) return;

        tft->fillScreen(TFT_BLACK);

        // --- Header ---
        tft->fillRect(0, 0, 240, 32, 0x2124);
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(TFT_WHITE, 0x2124);
        tft->drawString("SYSTEM INFO", 120, 16, 4);

        tft->setTextDatum(TL_DATUM);

        // --- Device info ---
        tft->setTextColor(TFT_CYAN, TFT_BLACK);
        tft->drawString("TARGET: Suraksha+", 10, 40, 2);

        // --- Memory ---
        char memBuf[40];
        snprintf(memBuf, sizeof(memBuf), "Heap: %d KB  PSRAM: %d KB",
                 (int)(ESP.getFreeHeap() / 1024),
                 (int)(ESP.getFreePsram() / 1024));
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->drawString(memBuf, 10, 62, 2);

        // --- CPU & Temperature ---
        char cpuBuf[40];
        snprintf(cpuBuf, sizeof(cpuBuf), "CPU: %d MHz  Temp: %.0f C",
                 ESP.getCpuFreqMHz(),
                 temperatureRead());
        tft->drawString(cpuBuf, 10, 84, 2);

        // --- ESP-NOW Comms ---
        ESPNowReceiver& espnow = ESPNowReceiver::getInstance();
        char espnowBuf[44];
        snprintf(espnowBuf, sizeof(espnowBuf), "ESP-NOW: %s (%lu rx)",
                 espnow.isSenderOnline() ? "ONLINE" : "SEARCHING",
                 (unsigned long)espnow.getPacketsReceived());
        tft->setTextColor(espnow.isSenderOnline() ? TFT_GREEN : TFT_ORANGE, TFT_BLACK);
        tft->drawString(espnowBuf, 10, 108, 2);

        // --- BLE Status ---
        bool bleConn = BleService::getInstance().isConnected();
        tft->setTextColor(bleConn ? TFT_GREEN : TFT_YELLOW, TFT_BLACK);
        tft->drawString(bleConn ? "BLE: Connected" : "BLE: Advertising", 10, 130, 2);

        // --- Uptime ---
        uint32_t uptimeSec = millis() / 1000;
        uint32_t uptimeMin = uptimeSec / 60;
        uptimeSec %= 60;
        uint32_t uptimeHr = uptimeMin / 60;
        uptimeMin %= 60;

        char uptimeBuf[36];
        if (uptimeHr > 0) {
            snprintf(uptimeBuf, sizeof(uptimeBuf), "Uptime: %luh %lum %lus",
                     (unsigned long)uptimeHr, (unsigned long)uptimeMin, (unsigned long)uptimeSec);
        } else {
            snprintf(uptimeBuf, sizeof(uptimeBuf), "Uptime: %lum %lus",
                     (unsigned long)uptimeMin, (unsigned long)uptimeSec);
        }
        tft->setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        tft->drawString(uptimeBuf, 10, 154, 2);

        // --- Divider ---
        tft->drawFastHLine(10, 180, 220, 0x39E7);

        // --- OS Version & Branding ---
        char verBuf[36];
        snprintf(verBuf, sizeof(verBuf), "Suraksha+ OS v%s", WATCH_OS_VERSION);
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(0x07FF, TFT_BLACK);
        tft->drawString(verBuf, 120, 198, 2);

        tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft->drawString("ESP-NOW Dual-Device Architecture", 120, 220, 1);
    }

} // namespace SystemInfoScreen
