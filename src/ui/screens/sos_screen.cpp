// ============================================================================
// Suraksha+ Watch OS — SOS Screen Implementation
// 128x64 OLED layout using U8g2 — inverted for maximum attention
// ============================================================================
#include "sos_screen.h"
#include "../../safety/safety_engine.h"
#include "../../sensors/sensor_manager.h"

namespace SOSScreen {

    void draw(U8G2_SH1106_128X64_NONAME_F_HW_I2C* u8g2) {
        SafetyEngine& safety = SafetyEngine::getInstance();
        SensorManager& sensors = SensorManager::getInstance();

        // --- Flashing effect: alternate between normal and inverted ---
        bool flash = (millis() / 500) % 2 == 0;

        // --- SOS ACTIVE banner (inverted rectangle for attention) ---
        if (flash) {
            u8g2->drawBox(0, 0, 128, 14);  // Filled rectangle
            u8g2->setDrawColor(0);          // Draw black text on white bg
        }
        u8g2->setFont(u8g2_font_7x14B_tr);
        u8g2->drawStr(20, 12, "!! SOS ACTIVE !!");
        u8g2->setDrawColor(1);  // Reset to white-on-black

        // --- Timer (duration since SOS) ---
        uint32_t durationMs = safety.getSOSManager().getDurationMs();
        uint32_t seconds = durationMs / 1000;
        uint32_t minutes = seconds / 60;
        seconds %= 60;

        char timerBuf[8];
        snprintf(timerBuf, sizeof(timerBuf), "%02d:%02d", minutes, seconds);

        u8g2->setFont(u8g2_font_logisoso16_tn);
        int timerWidth = u8g2->getStrWidth(timerBuf);
        u8g2->drawStr((128 - timerWidth) / 2, 34, timerBuf);

        // --- GPS coordinates ---
        u8g2->setFont(u8g2_font_5x7_tr);
        if (sensors.hasGPSFix()) {
            char gpsBuf[28];
            snprintf(gpsBuf, sizeof(gpsBuf), "GPS:%.4f,%.4f",
                     sensors.getLatitude(), sensors.getLongitude());
            u8g2->drawStr(0, 44, gpsBuf);
        } else {
            u8g2->drawStr(0, 44, "GPS: Acquiring...");
        }

        // --- Heart rate ---
        float bpm = sensors.getBPM();
        char bpmBuf[16];
        if (bpm > 0) {
            snprintf(bpmBuf, sizeof(bpmBuf), "BPM: %.0f", bpm);
        } else {
            snprintf(bpmBuf, sizeof(bpmBuf), "BPM: --");
        }
        u8g2->drawStr(0, 53, bpmBuf);

        // --- Cancel instruction ---
        u8g2->drawHLine(0, 55, 128);
        u8g2->setFont(u8g2_font_5x7_tr);
        const char* cancelMsg = "HOLD 5s TO CANCEL";
        int cancelWidth = u8g2->getStrWidth(cancelMsg);
        u8g2->drawStr((128 - cancelWidth) / 2, 63, cancelMsg);
    }

} // namespace SOSScreen
