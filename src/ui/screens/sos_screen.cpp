// ============================================================================
// Suraksha+ Watch OS — SOS Screen Implementation
// 240x240 Color TFT layout for LilyGo T-Watch S3
// ============================================================================
#include "sos_screen.h"
#include "../../safety/safety_engine.h"
#include "../../sensors/sensor_manager.h"

namespace SOSScreen {

    void draw(TFT_eSPI* tft) {
        if (!tft) return;

        SafetyEngine& safety = SafetyEngine::getInstance();
        SensorManager& sensors = SensorManager::getInstance();

        // Alternate background/header flashing for emergency attention
        bool flash = (millis() / 500) % 2 == 0;

        tft->fillScreen(TFT_BLACK);

        // --- Top Alert Banner ---
        uint16_t bannerBg = flash ? TFT_RED : 0x8800; // Bright red / dark red
        tft->fillRect(0, 0, 240, 50, bannerBg);

        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(TFT_WHITE, bannerBg);
        tft->drawString("! EMERGENCY SOS !", 120, 25, 4);

        // --- Timer (duration since SOS triggered) ---
        uint32_t durationMs = safety.getSOSManager().getDurationMs();
        uint32_t seconds = durationMs / 1000;
        uint32_t minutes = seconds / 60;
        seconds %= 60;

        char timerBuf[16];
        snprintf(timerBuf, sizeof(timerBuf), "%02lu:%02lu", (unsigned long)minutes, (unsigned long)seconds);

        tft->setTextColor(TFT_YELLOW, TFT_BLACK);
        tft->drawString(timerBuf, 120, 85, 6); // Large font

        // --- GPS Coordinates Card ---
        tft->fillRoundRect(15, 120, 210, 50, 6, 0x18E3);
        tft->drawRoundRect(15, 120, 210, 50, 6, TFT_RED);

        tft->setTextDatum(TL_DATUM);
        if (sensors.hasGPSFix()) {
            char latBuf[32];
            char lonBuf[32];
            snprintf(latBuf, sizeof(latBuf), "LAT: %.5f", sensors.getLatitude());
            snprintf(lonBuf, sizeof(lonBuf), "LON: %.5f", sensors.getLongitude());
            tft->setTextColor(TFT_GREEN, 0x18E3);
            tft->drawString(latBuf, 25, 126, 2);
            tft->drawString(lonBuf, 25, 146, 2);
        } else {
            tft->setTextColor(TFT_ORANGE, 0x18E3);
            tft->drawString("GPS: Acquiring fix...", 25, 126, 2);
            tft->setTextColor(TFT_DARKGREY, 0x18E3);
            tft->drawString("Broadcasting BLE beacon", 25, 146, 2);
        }

        // --- Heart Rate & Alert Info ---
        float bpm = sensors.getBPM();
        char hrBuf[32];
        if (bpm > 0) {
            snprintf(hrBuf, sizeof(hrBuf), "HEART RATE: %.0f BPM", bpm);
            tft->setTextColor(TFT_RED, TFT_BLACK);
        } else {
            snprintf(hrBuf, sizeof(hrBuf), "HEART RATE: ---");
            tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
        }
        tft->setTextDatum(MC_DATUM);
        tft->drawString(hrBuf, 120, 185, 2);

        // --- Instructions to cancel ---
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->drawString("HOLD 3 SECONDS TO CANCEL", 120, 215, 2);
    }

} // namespace SOSScreen
