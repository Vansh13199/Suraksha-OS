// ============================================================================
// Suraksha+ Watch OS — Health Screen Implementation
// 240x240 Color TFT layout for LilyGo T-Watch S3
// ============================================================================
#include "health_screen.h"
#include "../../sensors/sensor_manager.h"

namespace HealthScreen {

    void draw(TFT_eSPI* tft) {
        if (!tft) return;

        SensorManager& sensors = SensorManager::getInstance();

        tft->fillScreen(TFT_BLACK);

        // --- Header ---
        tft->fillRect(0, 0, 240, 36, 0x10A2); // Deep navy header
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(TFT_CYAN, 0x10A2);
        tft->drawString("HEALTH MONITOR", 120, 18, 4);

        // --- Heart Rate Card ---
        tft->fillRoundRect(10, 44, 220, 80, 8, 0x18E3);
        tft->drawRoundRect(10, 44, 220, 80, 8, TFT_RED);

        float bpm = sensors.getBPM();
        char bpmBuf[16];
        if (bpm > 0) {
            snprintf(bpmBuf, sizeof(bpmBuf), "%.0f", bpm);
        } else {
            snprintf(bpmBuf, sizeof(bpmBuf), "--");
        }

        tft->setTextDatum(TL_DATUM);
        tft->setTextColor(TFT_RED, 0x18E3);
        tft->drawString("HEART RATE", 20, 52, 2);

        // Large BPM number
        tft->setTextDatum(MC_DATUM);
        tft->drawString(bpmBuf, 100, 88, 6); // Large font

        tft->setTextDatum(TL_DATUM);
        tft->setTextColor(TFT_WHITE, 0x18E3);
        tft->drawString("BPM", 155, 80, 4);

        // Status indicator (SpO2 removed)
        bool finger = sensors.isHRSensorFingerPresent();
        tft->setTextDatum(TR_DATUM);
        if (finger) {
            tft->setTextColor(TFT_GREEN, 0x18E3);
            tft->drawString("FINGER ON", 220, 52, 2);
        } else if (bpm > 0) {
            tft->setTextColor(TFT_YELLOW, 0x18E3);
            tft->drawString("MAINTAINED", 220, 52, 2);
        } else {
            tft->setTextColor(TFT_DARKGREY, 0x18E3);
            tft->drawString("NO DATA", 220, 52, 2);
        }

        // --- Steps & Motion Card ---
        tft->fillRoundRect(10, 132, 220, 50, 8, 0x18E3);
        tft->drawRoundRect(10, 132, 220, 50, 8, TFT_ORANGE);

        uint32_t steps = sensors.getSteps();
        char stepsBuf[24];
        snprintf(stepsBuf, sizeof(stepsBuf), "Steps: %lu", (unsigned long)steps);
        tft->setTextDatum(TL_DATUM);
        tft->setTextColor(TFT_ORANGE, 0x18E3);
        tft->drawString(stepsBuf, 20, 140, 2);

        float motion = sensors.getMotionScore();
        char motBuf[24];
        snprintf(motBuf, sizeof(motBuf), "Motion: %.2f g", motion);
        tft->setTextColor(TFT_YELLOW, 0x18E3);
        tft->drawString(motBuf, 20, 160, 2);

        // --- Remote Seeed Sensor Info Bar ---
        bool online = sensors.isHRSensorOnline();
        tft->fillRoundRect(10, 190, 220, 42, 8, 0x0841);
        tft->drawRoundRect(10, 190, 220, 42, 8, online ? TFT_GREEN : TFT_DARKGREY);

        tft->setTextDatum(TL_DATUM);
        if (online) {
            tft->setTextColor(TFT_GREEN, 0x0841);
            tft->drawString("Seeed HR: LINKED", 20, 196, 2);
            char linkInfo[36];
            snprintf(linkInfo, sizeof(linkInfo), "Bat: %d%%  RSSI: %ddBm",
                     sensors.getHRSensorBattery(), sensors.getHRSensorRSSI());
            tft->setTextColor(TFT_WHITE, 0x0841);
            tft->drawString(linkInfo, 20, 212, 2);
        } else {
            tft->setTextColor(TFT_ORANGE, 0x0841);
            tft->drawString("Seeed HR: SEARCHING", 20, 196, 2);
            tft->setTextColor(TFT_DARKGREY, 0x0841);
            tft->drawString("Check sensor module power", 20, 212, 2);
        }
    }

} // namespace HealthScreen
