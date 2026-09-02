// ============================================================================
// Suraksha+ Watch OS — Watch Face Screen Implementation
// 128x64 OLED layout using U8g2
// ============================================================================
#include "watchface_screen.h"
#include "../../sensors/sensor_manager.h"
#include "../../safety/safety_engine.h"
#include "../../comms/ble_service.h"
#include <time.h>

namespace WatchfaceScreen {

    void draw(U8G2_SH1106_128X64_NONAME_F_HW_I2C* u8g2) {
        // --- Status bar (top row, y=0..10) ---
        u8g2->setFont(u8g2_font_5x7_tr);

        // Heart rate (top-left)
        SensorManager& sensors = SensorManager::getInstance();
        float bpm = sensors.getBPM();
        char bpmBuf[12];
        if (bpm > 0) {
            snprintf(bpmBuf, sizeof(bpmBuf), "%c%.0f", 0x03, bpm); // heart char
        } else {
            snprintf(bpmBuf, sizeof(bpmBuf), "%c--", 0x03);
        }
        u8g2->drawStr(0, 7, bpmBuf);

        // BLE status (center-left)
        bool bleConnected = BleService::getInstance().isConnected();
        u8g2->drawStr(40, 7, bleConnected ? "BLE*" : "BLE");

        // Battery (top-right)
        uint8_t batPct = sensors.getBatteryPercent();
        char batBuf[8];
        snprintf(batBuf, sizeof(batBuf), "%d%%", batPct);
        int batWidth = u8g2->getStrWidth(batBuf);
        u8g2->drawStr(128 - batWidth, 7, batBuf);

        // Separator line
        u8g2->drawHLine(0, 10, 128);

        // --- Time display (center, large) ---
        struct tm timeinfo;
        time_t now;
        time(&now);
        localtime_r(&now, &timeinfo);

        char timeBuf[8];
        strftime(timeBuf, sizeof(timeBuf), "%H:%M", &timeinfo);

        u8g2->setFont(u8g2_font_logisoso24_tn); // Large numeric font
        int timeWidth = u8g2->getStrWidth(timeBuf);
        u8g2->drawStr((128 - timeWidth) / 2, 40, timeBuf);

        // --- Date (below time) ---
        char dateBuf[20];
        strftime(dateBuf, sizeof(dateBuf), "%a, %b %d", &timeinfo);
        // Convert to uppercase
        for (int i = 0; dateBuf[i]; i++) dateBuf[i] = toupper(dateBuf[i]);

        u8g2->setFont(u8g2_font_5x7_tr);
        int dateWidth = u8g2->getStrWidth(dateBuf);
        u8g2->drawStr((128 - dateWidth) / 2, 50, dateBuf);

        // --- Bottom bar (y=55..64) ---
        u8g2->drawHLine(0, 54, 128);

        // Steps (bottom-left)
        uint32_t steps = sensors.getSteps();
        char stepBuf[16];
        snprintf(stepBuf, sizeof(stepBuf), "%d steps", steps);
        u8g2->drawStr(0, 63, stepBuf);

        // Safety status (bottom-right)
        SafetyEngine& safety = SafetyEngine::getInstance();
        if (safety.isSOSActive()) {
            u8g2->drawStr(90, 63, "!SOS!");
        } else if (safety.isActivityMode()) {
            u8g2->drawStr(88, 63, "ACTIV");
        } else {
            u8g2->drawStr(96, 63, "IDLE");
        }
    }

} // namespace WatchfaceScreen
