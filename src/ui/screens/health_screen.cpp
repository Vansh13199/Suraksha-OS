// ============================================================================
// Suraksha+ Watch OS — Health Screen Implementation
// 128x64 OLED layout using U8g2
// ============================================================================
#include "health_screen.h"
#include "../../sensors/sensor_manager.h"

namespace HealthScreen {

    void draw(U8G2_SH1106_128X64_NONAME_F_HW_I2C* u8g2) {
        SensorManager& sensors = SensorManager::getInstance();

        // --- Title ---
        u8g2->setFont(u8g2_font_6x10_tr);
        u8g2->drawStr(0, 9, "HEART RATE");
        u8g2->drawHLine(0, 11, 128);

        // --- BPM value (large) ---
        float bpm = sensors.getBPM();
        char bpmBuf[12];
        if (bpm > 0) {
            snprintf(bpmBuf, sizeof(bpmBuf), "%.0f", bpm);
        } else {
            snprintf(bpmBuf, sizeof(bpmBuf), "--");
        }

        u8g2->setFont(u8g2_font_logisoso20_tn);
        int bpmWidth = u8g2->getStrWidth(bpmBuf);
        u8g2->drawStr((128 - bpmWidth) / 2 - 10, 36, bpmBuf);

        // "BPM" label next to value
        u8g2->setFont(u8g2_font_6x10_tr);
        u8g2->drawStr((128 + bpmWidth) / 2 - 5, 36, "BPM");

        // Finger status
        bool fingerOn = sensors.isHeartRateAvailable() && bpm > 0;
        u8g2->setFont(u8g2_font_5x7_tr);
        u8g2->drawStr(0, 36, fingerOn ? "ON" : "OFF");

        // --- Separator ---
        u8g2->drawHLine(0, 39, 128);

        // --- Steps ---
        uint32_t steps = sensors.getSteps();
        char stepBuf[20];
        snprintf(stepBuf, sizeof(stepBuf), "Steps: %d", steps);
        u8g2->setFont(u8g2_font_6x10_tr);
        u8g2->drawStr(0, 51, stepBuf);

        // --- Motion score ---
        float motion = sensors.getMotionScore();
        char motBuf[20];
        snprintf(motBuf, sizeof(motBuf), "Motion: %.2f", motion);
        u8g2->drawStr(0, 63, motBuf);

        // --- GPS status (right side) ---
        if (sensors.hasGPSFix()) {
            char gpsBuf[16];
            snprintf(gpsBuf, sizeof(gpsBuf), "GPS OK");
            u8g2->drawStr(88, 51, gpsBuf);
        } else {
            u8g2->drawStr(82, 51, "No GPS");
        }

        // --- Battery (bottom-right) ---
        uint8_t batPct = sensors.getBatteryPercent();
        char batBuf[8];
        snprintf(batBuf, sizeof(batBuf), "%d%%", batPct);
        int w = u8g2->getStrWidth(batBuf);
        u8g2->drawStr(128 - w, 63, batBuf);
    }

} // namespace HealthScreen
