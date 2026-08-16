// ============================================================================
// Suraksha+ Watch OS — Watch Face Screen Implementation
// ============================================================================
#include "watchface_screen.h"
#include "../theme.h"
#include "../../sensors/sensor_manager.h"
#include "../../safety/safety_engine.h"
#include "../../comms/ble_service.h"
#include "../../config/hardware_config.h"
#include <time.h>

// Static UI element references
static lv_obj_t* lblTime = nullptr;
static lv_obj_t* lblDate = nullptr;
static lv_obj_t* lblBPM = nullptr;
static lv_obj_t* lblSteps = nullptr;
static lv_obj_t* lblBattery = nullptr;
static lv_obj_t* lblBLE = nullptr;
static lv_obj_t* sosIndicator = nullptr;
static lv_obj_t* activityIndicator = nullptr;
static lv_timer_t* updateTimer = nullptr;

// Timer callback to refresh watch face data
static void _updateTimerCb(lv_timer_t* timer) {
    lv_obj_t* screen = (lv_obj_t*)timer->user_data;
    WatchfaceScreen::update(screen);
}

lv_obj_t* WatchfaceScreen::create() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, Theme::COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // --- Circular clipping mask (visual boundary) ---
    // The round display naturally clips, but we keep content within circle

    // --- Battery icon (top right area) ---
    lblBattery = lv_label_create(scr);
    lv_obj_set_style_text_font(lblBattery, Theme::FONT_SMALL, 0);
    lv_obj_set_style_text_color(lblBattery, Theme::COLOR_BATTERY, 0);
    lv_obj_align(lblBattery, LV_ALIGN_TOP_RIGHT, -40, 20);
    lv_label_set_text(lblBattery, "100%");

    // --- BLE status (top left area) ---
    lblBLE = lv_label_create(scr);
    lv_obj_set_style_text_font(lblBLE, Theme::FONT_SMALL, 0);
    lv_obj_set_style_text_color(lblBLE, Theme::COLOR_BLE, 0);
    lv_obj_align(lblBLE, LV_ALIGN_TOP_LEFT, 40, 20);
    lv_label_set_text(lblBLE, LV_SYMBOL_BLUETOOTH);

    // --- Time display (center, large) ---
    lblTime = lv_label_create(scr);
    lv_obj_set_style_text_font(lblTime, Theme::FONT_TIME, 0);
    lv_obj_set_style_text_color(lblTime, Theme::COLOR_TEXT, 0);
    lv_obj_set_style_text_letter_space(lblTime, 2, 0);
    lv_obj_align(lblTime, LV_ALIGN_CENTER, 0, -15);
    lv_label_set_text(lblTime, "12:00");

    // --- Date display (below time) ---
    lblDate = lv_label_create(scr);
    lv_obj_set_style_text_font(lblDate, Theme::FONT_NORMAL, 0);
    lv_obj_set_style_text_color(lblDate, Theme::COLOR_TEXT_DIM, 0);
    lv_obj_align(lblDate, LV_ALIGN_CENTER, 0, 25);
    lv_label_set_text(lblDate, "SAT, AUG 16");

    // --- Heart rate (bottom left) ---
    lblBPM = lv_label_create(scr);
    lv_obj_set_style_text_font(lblBPM, Theme::FONT_NORMAL, 0);
    lv_obj_set_style_text_color(lblBPM, Theme::COLOR_HEART, 0);
    lv_obj_align(lblBPM, LV_ALIGN_BOTTOM_LEFT, 50, -45);
    lv_label_set_text(lblBPM, LV_SYMBOL_WARNING " --");

    // --- Steps (bottom right) ---
    lblSteps = lv_label_create(scr);
    lv_obj_set_style_text_font(lblSteps, Theme::FONT_NORMAL, 0);
    lv_obj_set_style_text_color(lblSteps, Theme::COLOR_TEXT_DIM, 0);
    lv_obj_align(lblSteps, LV_ALIGN_BOTTOM_RIGHT, -50, -45);
    lv_label_set_text(lblSteps, "0 steps");

    // --- SOS Active indicator (pulsing red ring) ---
    sosIndicator = lv_obj_create(scr);
    lv_obj_set_size(sosIndicator, DISPLAY_WIDTH - 4, DISPLAY_HEIGHT - 4);
    lv_obj_align(sosIndicator, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(sosIndicator, DISPLAY_RADIUS, 0);
    lv_obj_set_style_bg_opa(sosIndicator, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(sosIndicator, Theme::COLOR_DANGER, 0);
    lv_obj_set_style_border_width(sosIndicator, 3, 0);
    lv_obj_set_style_border_opa(sosIndicator, LV_OPA_COVER, 0);
    lv_obj_clear_flag(sosIndicator, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(sosIndicator, LV_OBJ_FLAG_HIDDEN); // Hidden by default

    // --- Activity Mode indicator (orange arc at bottom) ---
    activityIndicator = lv_label_create(scr);
    lv_obj_set_style_text_font(activityIndicator, Theme::FONT_SMALL, 0);
    lv_obj_set_style_text_color(activityIndicator, Theme::COLOR_ACTIVITY, 0);
    lv_obj_align(activityIndicator, LV_ALIGN_BOTTOM_MID, 0, -25);
    lv_label_set_text(activityIndicator, "");
    lv_obj_add_flag(activityIndicator, LV_OBJ_FLAG_HIDDEN);

    // --- Auto-update timer (1 second interval) ---
    updateTimer = lv_timer_create(_updateTimerCb, 1000, scr);

    return scr;
}

void WatchfaceScreen::update(lv_obj_t* screen) {
    if (!screen) return;

    // --- Update time ---
    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);

    char timeBuf[8];
    strftime(timeBuf, sizeof(timeBuf), "%H:%M", &timeinfo);
    if (lblTime) lv_label_set_text(lblTime, timeBuf);

    // --- Update date ---
    char dateBuf[20];
    strftime(dateBuf, sizeof(dateBuf), "%a, %b %d", &timeinfo);
    // Convert to uppercase
    for (int i = 0; dateBuf[i]; i++) dateBuf[i] = toupper(dateBuf[i]);
    if (lblDate) lv_label_set_text(lblDate, dateBuf);

    // --- Update heart rate ---
    SensorManager& sensors = SensorManager::getInstance();
    float bpm = sensors.getBPM();
    if (bpm > 0) {
        char bpmBuf[16];
        snprintf(bpmBuf, sizeof(bpmBuf), LV_SYMBOL_WARNING " %.0f", bpm);
        if (lblBPM) lv_label_set_text(lblBPM, bpmBuf);
    } else {
        if (lblBPM) lv_label_set_text(lblBPM, LV_SYMBOL_WARNING " --");
    }

    // --- Update steps ---
    uint32_t steps = sensors.getSteps();
    char stepBuf[16];
    snprintf(stepBuf, sizeof(stepBuf), "%d steps", steps);
    if (lblSteps) lv_label_set_text(lblSteps, stepBuf);

    // --- Update battery ---
    uint8_t batPct = sensors.getBatteryPercent();
    char batBuf[8];
    snprintf(batBuf, sizeof(batBuf), "%d%%", batPct);
    if (lblBattery) {
        lv_label_set_text(lblBattery, batBuf);
        lv_obj_set_style_text_color(lblBattery,
            batPct <= 20 ? Theme::COLOR_BATTERY_LOW : Theme::COLOR_BATTERY, 0);
    }

    // --- Update BLE status ---
    bool bleConnected = BleService::getInstance().isConnected();
    if (lblBLE) {
        lv_obj_set_style_text_color(lblBLE,
            bleConnected ? Theme::COLOR_BLE : Theme::COLOR_TEXT_MUTED, 0);
    }

    // --- SOS indicator ---
    SafetyEngine& safety = SafetyEngine::getInstance();
    if (sosIndicator) {
        if (safety.isSOSActive()) {
            lv_obj_clear_flag(sosIndicator, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(sosIndicator, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // --- Activity mode indicator ---
    if (activityIndicator) {
        if (safety.isActivityMode()) {
            uint32_t remaining = safety.getActivityMode().getRemainingSeconds();
            char actBuf[32];
            snprintf(actBuf, sizeof(actBuf), "ACTIVITY %d:%02d",
                     remaining / 60, remaining % 60);
            lv_label_set_text(activityIndicator, actBuf);
            lv_obj_clear_flag(activityIndicator, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(activityIndicator, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void WatchfaceScreen::destroy(lv_obj_t* screen) {
    if (updateTimer) {
        lv_timer_del(updateTimer);
        updateTimer = nullptr;
    }
    lblTime = nullptr;
    lblDate = nullptr;
    lblBPM = nullptr;
    lblSteps = nullptr;
    lblBattery = nullptr;
    lblBLE = nullptr;
    sosIndicator = nullptr;
    activityIndicator = nullptr;
}
