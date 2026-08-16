// ============================================================================
// Suraksha+ Watch OS — SOS Screen Implementation
// ============================================================================
#include "sos_screen.h"
#include "../theme.h"
#include "../../safety/safety_engine.h"
#include "../../sensors/sensor_manager.h"

static lv_obj_t* lblSOSTitle = nullptr;
static lv_obj_t* lblSOSTimer = nullptr;
static lv_obj_t* lblSOSGPS = nullptr;
static lv_obj_t* lblSOSBPM = nullptr;
static lv_obj_t* lblSOSCancel = nullptr;
static lv_timer_t* sosTimer = nullptr;

static void _sosTimerCb(lv_timer_t* timer) {
    lv_obj_t* screen = (lv_obj_t*)timer->user_data;
    SOSScreen::update(screen);
}

lv_obj_t* SOSScreen::create() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, Theme::COLOR_DANGER_DARK, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // --- SOS Title (pulsing) ---
    lblSOSTitle = lv_label_create(scr);
    lv_obj_set_style_text_font(lblSOSTitle, Theme::FONT_TITLE, 0);
    lv_obj_set_style_text_color(lblSOSTitle, Theme::COLOR_TEXT, 0);
    lv_obj_align(lblSOSTitle, LV_ALIGN_CENTER, 0, -45);
    lv_label_set_text(lblSOSTitle, "SOS ACTIVE");

    // Pulsing animation on title
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, lblSOSTitle);
    lv_anim_set_values(&anim, LV_OPA_COVER, LV_OPA_50);
    lv_anim_set_time(&anim, 800);
    lv_anim_set_playback_time(&anim, 800);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&anim, [](void* obj, int32_t v) {
        lv_obj_set_style_text_opa((lv_obj_t*)obj, v, 0);
    });
    lv_anim_start(&anim);

    // --- Timer (duration since SOS) ---
    lblSOSTimer = lv_label_create(scr);
    lv_obj_set_style_text_font(lblSOSTimer, Theme::FONT_XLARGE, 0);
    lv_obj_set_style_text_color(lblSOSTimer, Theme::COLOR_TEXT, 0);
    lv_obj_align(lblSOSTimer, LV_ALIGN_CENTER, 0, -5);
    lv_label_set_text(lblSOSTimer, "00:00");

    // --- GPS coordinates ---
    lblSOSGPS = lv_label_create(scr);
    lv_obj_set_style_text_font(lblSOSGPS, Theme::FONT_SMALL, 0);
    lv_obj_set_style_text_color(lblSOSGPS, lv_color_hex(0xFFCDD2), 0);
    lv_obj_align(lblSOSGPS, LV_ALIGN_CENTER, 0, 25);
    lv_label_set_text(lblSOSGPS, "GPS: --");

    // --- Heart rate ---
    lblSOSBPM = lv_label_create(scr);
    lv_obj_set_style_text_font(lblSOSBPM, Theme::FONT_NORMAL, 0);
    lv_obj_set_style_text_color(lblSOSBPM, lv_color_hex(0xFFCDD2), 0);
    lv_obj_align(lblSOSBPM, LV_ALIGN_CENTER, 0, 45);
    lv_label_set_text(lblSOSBPM, "BPM: --");

    // --- Cancel instruction ---
    lblSOSCancel = lv_label_create(scr);
    lv_obj_set_style_text_font(lblSOSCancel, Theme::FONT_SMALL, 0);
    lv_obj_set_style_text_color(lblSOSCancel, lv_color_hex(0xFFAB91), 0);
    lv_obj_align(lblSOSCancel, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_label_set_text(lblSOSCancel, "HOLD 5s TO CANCEL");

    // --- Update timer ---
    sosTimer = lv_timer_create(_sosTimerCb, 1000, scr);

    return scr;
}

void SOSScreen::update(lv_obj_t* screen) {
    if (!screen) return;

    SafetyEngine& safety = SafetyEngine::getInstance();
    SensorManager& sensors = SensorManager::getInstance();

    // Update duration timer
    uint32_t durationMs = safety.getSOSManager().getDurationMs();
    uint32_t seconds = durationMs / 1000;
    uint32_t minutes = seconds / 60;
    seconds %= 60;

    char timerBuf[8];
    snprintf(timerBuf, sizeof(timerBuf), "%02d:%02d", minutes, seconds);
    if (lblSOSTimer) lv_label_set_text(lblSOSTimer, timerBuf);

    // Update GPS
    if (sensors.hasGPSFix()) {
        char gpsBuf[32];
        snprintf(gpsBuf, sizeof(gpsBuf), "GPS: %.4f, %.4f",
                 sensors.getLatitude(), sensors.getLongitude());
        if (lblSOSGPS) lv_label_set_text(lblSOSGPS, gpsBuf);
    } else {
        if (lblSOSGPS) lv_label_set_text(lblSOSGPS, "GPS: Acquiring...");
    }

    // Update BPM
    float bpm = sensors.getBPM();
    char bpmBuf[16];
    if (bpm > 0) {
        snprintf(bpmBuf, sizeof(bpmBuf), "BPM: %.0f", bpm);
    } else {
        snprintf(bpmBuf, sizeof(bpmBuf), "BPM: --");
    }
    if (lblSOSBPM) lv_label_set_text(lblSOSBPM, bpmBuf);
}

void SOSScreen::destroy(lv_obj_t* screen) {
    if (sosTimer) {
        lv_timer_del(sosTimer);
        sosTimer = nullptr;
    }
    lblSOSTitle = nullptr;
    lblSOSTimer = nullptr;
    lblSOSGPS = nullptr;
    lblSOSBPM = nullptr;
    lblSOSCancel = nullptr;
}
