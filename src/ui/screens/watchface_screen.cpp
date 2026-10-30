// ============================================================================
// Suraksha+ Watch OS — Watch Face Screen Implementation
// 240x240 Color TFT layout for LilyGo T-Watch S3
// ============================================================================
#include "watchface_screen.h"
#include "../../sensors/sensor_manager.h"
#include "../../safety/safety_engine.h"
#include "../../comms/ble_service.h"
#include <time.h>
#include <WiFi.h>

namespace WatchfaceScreen {

    void draw(TFT_eSPI* tft) {
        if (!tft) return;

        SafetyEngine& safety = SafetyEngine::getInstance();
        SensorManager& sensors = SensorManager::getInstance();

        tft->fillScreen(TFT_BLACK);

        if (safety.isActivityMode()) {
            // ================================================================
            // Activity Mode Screen
            // ================================================================
            tft->fillRoundRect(10, 10, 220, 36, 6, TFT_ORANGE);
            tft->setTextColor(TFT_BLACK, TFT_ORANGE);
            tft->setTextDatum(MC_DATUM);
            tft->drawString("ACTIVITY MODE", 120, 28, 4);

            // BPM display
            float bpm = sensors.getBPM();
            char bpmBuf[24];
            if (bpm > 0) {
                snprintf(bpmBuf, sizeof(bpmBuf), "BPM: %.0f", bpm);
            } else {
                snprintf(bpmBuf, sizeof(bpmBuf), "BPM: ---");
            }
            tft->setTextColor(TFT_RED, TFT_BLACK);
            tft->drawString(bpmBuf, 120, 80, 4);

            // Reverse timer
            uint32_t remaining = safety.getActivityMode().getRemainingSeconds();
            char timeBuf[16];
            tft->setTextColor(TFT_YELLOW, TFT_BLACK);

            if (remaining >= 3600) {
                uint32_t hours = remaining / 3600;
                uint32_t mins = (remaining % 3600) / 60;
                uint32_t secs = remaining % 60;
                snprintf(timeBuf, sizeof(timeBuf), "%02lu:%02lu:%02lu", (unsigned long)hours, (unsigned long)mins, (unsigned long)secs);
                tft->drawString(timeBuf, 120, 145, 6); // Font 6 fits HH:MM:SS cleanly
            } else {
                uint32_t mins = remaining / 60;
                uint32_t secs = remaining % 60;
                snprintf(timeBuf, sizeof(timeBuf), "%02lu:%02lu", (unsigned long)mins, (unsigned long)secs);
                tft->drawString(timeBuf, 120, 145, 7); // Font 7 = large 7-segment digital font
            }

            // Instruction
            tft->setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            tft->drawString("Tap 3x to finish", 120, 210, 2);
            return;
        }

        // ====================================================================
        // Normal Watch Face Mode (240x240)
        // ====================================================================

        // --- Status Bar (Top) ---
        // Heart Rate (Top Left) — SpO2 removed
        float bpm = sensors.getBPM();
        char hrBuf[24];
        if (bpm > 0) {
            snprintf(hrBuf, sizeof(hrBuf), "HR: %.0f", bpm);
            tft->setTextColor(TFT_RED, TFT_BLACK);
        } else {
            snprintf(hrBuf, sizeof(hrBuf), "HR: --");
            tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
        }
        tft->setTextDatum(TL_DATUM);
        tft->drawString(hrBuf, 10, 8, 2);

        // WiFi status (Top, shown when connected)
        bool wifiConn = (WiFi.status() == WL_CONNECTED);
        if (wifiConn) {
            // Draw WiFi vector wave icon
            int16_t wx = 98, wy = 20;
            uint16_t wifiColor = 0x07FF; // Bright Cyan
            tft->fillCircle(wx, wy, 2, wifiColor);
            tft->drawCircle(wx, wy, 5, wifiColor);
            tft->drawCircle(wx, wy, 8, wifiColor);
            tft->fillRect(wx - 9, wy + 1, 19, 9, TFT_BLACK);
            tft->fillRect(wx - 9, wy - 9, 4, 10, TFT_BLACK);
            tft->fillRect(wx + 6, wy - 9, 4, 10, TFT_BLACK);

            tft->setTextDatum(TL_DATUM);
            tft->setTextColor(wifiColor, TFT_BLACK);
            tft->drawString("WiFi", 110, 8, 2);
        }

        // BLE status (Top Center)
        bool bleConn = BleService::getInstance().isConnected();
        tft->setTextDatum(TC_DATUM);
        int16_t bleX = wifiConn ? 160 : 145;
        if (bleConn) {
            tft->setTextColor(TFT_CYAN, TFT_BLACK);
            tft->drawString("BLE", bleX, 8, 2);
        } else {
            tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
            tft->drawString("BLE", bleX, 8, 2);
        }

        // Watch Battery (Top Right)
        uint8_t batPct = sensors.getBatteryPercent();
        char batBuf[12];
        snprintf(batBuf, sizeof(batBuf), "%d%%", batPct);
        tft->setTextDatum(TR_DATUM);
        tft->setTextColor(batPct > 20 ? TFT_GREEN : TFT_RED, TFT_BLACK);
        tft->drawString(batBuf, 230, 8, 2);

        // Top divider
        tft->drawFastHLine(10, 28, 220, 0x39E7); // Dark grey line

        // --- 12-Hour IST Real Time Display ---
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);

        int rawHour = t ? t->tm_hour : 12;
        int rawMin  = t ? t->tm_min  : 0;
        int hour12  = rawHour % 12;
        if (hour12 == 0) hour12 = 12;
        const char* ampm = (rawHour >= 12) ? "PM" : "AM";

        char timeStr[16];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", hour12, rawMin);

        const char* days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
        const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
        char dateStr[32];
        if (t && t->tm_year > 100) {
            snprintf(dateStr, sizeof(dateStr), "%s, %02d %s",
                     days[t->tm_wday % 7], t->tm_mday, months[t->tm_mon % 12]);
        } else {
            snprintf(dateStr, sizeof(dateStr), "INDIA (IST)");
        }

        // Draw 12-hour digits in Font 7 (Large 7-segment digits, right-aligned at 160)
        tft->setTextDatum(MR_DATUM);
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->drawString(timeStr, 160, 75, 7);

        // Draw AM / PM in Font 4 (Bright Cyan) and IST indicator in Font 2 (Muted)
        tft->setTextDatum(ML_DATUM);
        tft->setTextColor(0x07FF, TFT_BLACK); // Bright Cyan
        tft->drawString(ampm, 168, 62, 4);

        tft->setTextColor(0x7BEF, TFT_BLACK); // Light Grey
        tft->drawString("IST", 168, 88, 2);

        // Date Display (Centered at 120, 118)
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(0x07FF, TFT_BLACK); // Cyan
        tft->drawString(dateStr, 120, 118, 2);

        // Middle divider
        tft->drawFastHLine(20, 136, 200, 0x39E7);

        // --- Remote Seeed Sensor Status Card ---
        bool sensorOnline = sensors.isHRSensorOnline();
        tft->fillRoundRect(15, 144, 210, 40, 6, 0x18E3); // Dark slate card
        tft->drawRoundRect(15, 144, 210, 40, 6, 0x39E7);

        tft->setTextDatum(TL_DATUM);
        if (sensorOnline) {
            tft->setTextColor(TFT_GREEN, 0x18E3);
            tft->drawString("SEEED SENSOR: ONLINE", 25, 150, 2);
            char linkBuf[32];
            snprintf(linkBuf, sizeof(linkBuf), "Bat: %d%%  RSSI: %ddBm",
                     sensors.getHRSensorBattery(), sensors.getHRSensorRSSI());
            tft->setTextColor(TFT_WHITE, 0x18E3);
            tft->drawString(linkBuf, 25, 166, 2);
        } else {
            tft->setTextColor(TFT_YELLOW, 0x18E3);
            tft->drawString("SEEED SENSOR: LINKING...", 25, 150, 2);
            tft->setTextColor(TFT_DARKGREY, 0x18E3);
            tft->drawString("Waiting for ESP-NOW...", 25, 166, 2);
        }

        // --- Bottom Activity / Step & Motion Card ---
        uint32_t steps = sensors.getSteps();
        float motion = sensors.getMotionScore();
        char stepsBuf[32];
        snprintf(stepsBuf, sizeof(stepsBuf), "STEPS:%lu  MOT:%.2f", (unsigned long)steps, motion);
        tft->setTextDatum(TL_DATUM);
        tft->setTextColor(TFT_ORANGE, TFT_BLACK);
        tft->drawString(stepsBuf, 15, 195, 2);

        // Safety Status badge
        tft->setTextDatum(TR_DATUM);
        if (safety.isSOSActive()) {
            tft->setTextColor(TFT_RED, TFT_BLACK);
            tft->drawString("!SOS!", 225, 195, 2);
        } else if (safety.isActivityMode()) {
            tft->setTextColor(TFT_YELLOW, TFT_BLACK);
            tft->drawString("ACT", 225, 195, 2);
        } else {
            tft->setTextColor(TFT_GREEN, TFT_BLACK);
            tft->drawString("SAFE", 225, 195, 2);
        }

        // --- Dynamic Bottom Gesture / Tap Prompt ---
        tft->setTextDatum(MC_DATUM);
        GestureDetector& gest = safety.getGestureDetector();
        if (gest.isWaitingToFinalize() && gest.getTapCount() > 0) {
            int tc = gest.getTapCount();
            char tapBuf[32];
            if (tc == 1) snprintf(tapBuf, sizeof(tapBuf), "👆 TAP 1... (Screen)");
            else if (tc == 2) snprintf(tapBuf, sizeof(tapBuf), "🚨 TAP 2... (SOS)");
            else if (tc == 3) snprintf(tapBuf, sizeof(tapBuf), "🏃 TAP 3... (Act OFF)");
            else snprintf(tapBuf, sizeof(tapBuf), "🏃 TAP %d... (Act ON)", tc);
            tft->setTextColor(TFT_YELLOW, TFT_BLACK);
            tft->drawString(tapBuf, 120, 225, 2);
        } else if (safety.isSOSActive()) {
            tft->setTextColor(TFT_RED, TFT_BLACK);
            tft->drawString("🚨 SOS ACTIVE! Hold 5s to cancel", 120, 225, 2);
        } else {
            tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
            tft->drawString("Swipe: Screens | 2x: SOS | 4x: Act", 120, 225, 1);
        }
    }

} // namespace WatchfaceScreen
