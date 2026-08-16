// ============================================================================
// Suraksha+ Watch OS — Health Screen Implementation
// ============================================================================
#include "health_screen.h"
#include "../theme.h"
#include "../../sensors/sensor_manager.h"

static lv_obj_t* lblHRTitle = nullptr;
static lv_obj_t* lblHRValue = nullptr;
static lv_obj_t* arcHR = nullptr;
static lv_obj_t* lblStepsTitle = nullptr;
static lv_obj_t* lblStepsValue = nullptr;
static lv_obj_t* arcSteps = nullptr;
static lv_obj_t* lblMotion = nullptr;
static lv_timer_t* healthTimer = nullptr;

static void _healthTimerCb(lv_timer_t* timer) {
    lv_obj_t* screen = (lv_obj_t*)timer->user_data;
    HealthScreen::update(screen);
}

lv_obj_t* HealthScreen::create() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, Theme::COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // --- Heart Rate Section (top half) ---
    // HR Arc background
    arcHR = lv_arc_create(scr);
    lv_obj_set_size(arcHR, 140, 140);
    lv_obj_align(arcHR, LV_ALIGN_CENTER, 0, -35);
    lv_arc_set_range(arcHR, 30, 200);
    lv_arc_set_value(arcHR, 72);
    lv_arc_set_bg_angles(arcHR, 135, 405);
    lv_obj_set_style_arc_color(arcHR, Theme::COLOR_SURFACE_LIGHT, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arcHR, Theme::COLOR_HEART, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arcHR, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arcHR, 8, LV_PART_INDICATOR);
    lv_obj_clear_flag(arcHR, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(arcHR, LV_OPA_TRANSP, LV_PART_KNOB);

    // HR Title
    lblHRTitle = lv_label_create(scr);
    lv_obj_set_style_text_font(lblHRTitle, Theme::FONT_SMALL, 0);
    lv_obj_set_style_text_color(lblHRTitle, Theme::COLOR_TEXT_DIM, 0);
    lv_obj_align(lblHRTitle, LV_ALIGN_CENTER, 0, -55);
    lv_label_set_text(lblHRTitle, LV_SYMBOL_WARNING " HEART RATE");

    // HR Value
    lblHRValue = lv_label_create(scr);
    lv_obj_set_style_text_font(lblHRValue, Theme::FONT_TITLE, 0);
    lv_obj_set_style_text_color(lblHRValue, Theme::COLOR_HEART, 0);
    lv_obj_align(lblHRValue, LV_ALIGN_CENTER, 0, -30);
    lv_label_set_text(lblHRValue, "--");

    // --- Steps Section (bottom half) ---
    lblStepsTitle = lv_label_create(scr);
    lv_obj_set_style_text_font(lblStepsTitle, Theme::FONT_SMALL, 0);
    lv_obj_set_style_text_color(lblStepsTitle, Theme::COLOR_TEXT_DIM, 0);
    lv_obj_align(lblStepsTitle, LV_ALIGN_CENTER, 0, 40);
    lv_label_set_text(lblStepsTitle, "STEPS TODAY");

    lblStepsValue = lv_label_create(scr);
    lv_obj_set_style_text_font(lblStepsValue, Theme::FONT_XLARGE, 0);
    lv_obj_set_style_text_color(lblStepsValue, Theme::COLOR_PRIMARY, 0);
    lv_obj_align(lblStepsValue, LV_ALIGN_CENTER, 0, 60);
    lv_label_set_text(lblStepsValue, "0");

    // --- Motion score ---
    lblMotion = lv_label_create(scr);
    lv_obj_set_style_text_font(lblMotion, Theme::FONT_SMALL, 0);
    lv_obj_set_style_text_color(lblMotion, Theme::COLOR_TEXT_MUTED, 0);
    lv_obj_align(lblMotion, LV_ALIGN_BOTTOM_MID, 0, -25);
    lv_label_set_text(lblMotion, "Motion: 0.00");

    // Update timer
    healthTimer = lv_timer_create(_healthTimerCb, 1000, scr);

    return scr;
}

void HealthScreen::update(lv_obj_t* screen) {
    if (!screen) return;

    SensorManager& sensors = SensorManager::getInstance();
    float bpm = sensors.getBPM();
    uint32_t steps = sensors.getSteps();
    float motion = sensors.getMotionScore();

    // Update HR
    if (bpm > 0) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%.0f", bpm);
        if (lblHRValue) lv_label_set_text(lblHRValue, buf);
        if (arcHR) lv_arc_set_value(arcHR, (int)bpm);
    } else {
        if (lblHRValue) lv_label_set_text(lblHRValue, "--");
    }

    // Update steps
    char stepBuf[12];
    snprintf(stepBuf, sizeof(stepBuf), "%d", steps);
    if (lblStepsValue) lv_label_set_text(lblStepsValue, stepBuf);

    // Update motion
    char motBuf[20];
    snprintf(motBuf, sizeof(motBuf), "Motion: %.2f", motion);
    if (lblMotion) lv_label_set_text(lblMotion, motBuf);
}

void HealthScreen::destroy(lv_obj_t* screen) {
    if (healthTimer) { lv_timer_del(healthTimer); healthTimer = nullptr; }
    lblHRTitle = nullptr; lblHRValue = nullptr; arcHR = nullptr;
    lblStepsTitle = nullptr; lblStepsValue = nullptr; lblMotion = nullptr;
}
