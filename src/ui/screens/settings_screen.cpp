// ============================================================================
// Suraksha+ Watch OS — Settings Screen Implementation
// ============================================================================
#include "settings_screen.h"
#include "../theme.h"
#include "../../platform/power_manager.h"
#include "../../platform/storage.h"
#include "../../platform/system_monitor.h"
#include "../../config/app_config.h"

static lv_obj_t* sliderBrightness = nullptr;

static void _brightnessCb(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    int val = lv_slider_get_value(slider);
    PowerManager::getInstance().setDisplayBrightness((uint8_t)val);
}

lv_obj_t* SettingsScreen::create() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, Theme::COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Title
    lv_obj_t* title = lv_label_create(scr);
    lv_obj_set_style_text_font(title, Theme::FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title, Theme::COLOR_TEXT, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_label_set_text(title, LV_SYMBOL_SETTINGS " Settings");

    // Scrollable container
    lv_obj_t* cont = lv_obj_create(scr);
    lv_obj_set_size(cont, 200, 170);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(cont, 8, 0);
    lv_obj_set_style_pad_all(cont, 4, 0);

    // --- Brightness ---
    lv_obj_t* lblBright = lv_label_create(cont);
    lv_obj_set_style_text_font(lblBright, Theme::FONT_SMALL, 0);
    lv_obj_set_style_text_color(lblBright, Theme::COLOR_TEXT_DIM, 0);
    lv_label_set_text(lblBright, "Brightness");

    sliderBrightness = lv_slider_create(cont);
    lv_obj_set_width(sliderBrightness, 180);
    lv_slider_set_range(sliderBrightness, 10, 255);
    lv_slider_set_value(sliderBrightness,
        PowerManager::getInstance().getDisplayBrightness(), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sliderBrightness, Theme::COLOR_SURFACE_LIGHT, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sliderBrightness, Theme::COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sliderBrightness, Theme::COLOR_PRIMARY, LV_PART_KNOB);
    lv_obj_add_event_cb(sliderBrightness, _brightnessCb, LV_EVENT_VALUE_CHANGED, NULL);

    // --- Power Save Toggle ---
    lv_obj_t* lblPwr = lv_label_create(cont);
    lv_obj_set_style_text_font(lblPwr, Theme::FONT_SMALL, 0);
    lv_obj_set_style_text_color(lblPwr, Theme::COLOR_TEXT_DIM, 0);
    lv_label_set_text(lblPwr, "Power Saving");

    lv_obj_t* swPower = lv_switch_create(cont);
    if (PowerManager::getInstance().isPowerSaveMode()) {
        lv_obj_add_state(swPower, LV_STATE_CHECKED);
    }
    lv_obj_set_style_bg_color(swPower, Theme::COLOR_SURFACE_LIGHT, LV_PART_MAIN);
    lv_obj_set_style_bg_color(swPower, Theme::COLOR_SAFETY, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(swPower, [](lv_event_t* e) {
        bool on = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
        PowerManager::getInstance().setPowerSaveMode(on);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // --- DND Toggle ---
    lv_obj_t* lblDND = lv_label_create(cont);
    lv_obj_set_style_text_font(lblDND, Theme::FONT_SMALL, 0);
    lv_obj_set_style_text_color(lblDND, Theme::COLOR_TEXT_DIM, 0);
    lv_label_set_text(lblDND, "Do Not Disturb");

    lv_obj_t* swDND = lv_switch_create(cont);
    if (Storage::getInstance().getBool(NVS_KEY_DND_MODE, false)) {
        lv_obj_add_state(swDND, LV_STATE_CHECKED);
    }
    lv_obj_set_style_bg_color(swDND, Theme::COLOR_SURFACE_LIGHT, LV_PART_MAIN);
    lv_obj_set_style_bg_color(swDND, Theme::COLOR_ACTIVITY, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(swDND, [](lv_event_t* e) {
        bool on = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
        Storage::getInstance().putBool(NVS_KEY_DND_MODE, on);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // --- Version info ---
    lv_obj_t* lblVer = lv_label_create(cont);
    lv_obj_set_style_text_font(lblVer, Theme::FONT_SMALL, 0);
    lv_obj_set_style_text_color(lblVer, Theme::COLOR_TEXT_MUTED, 0);
    lv_label_set_text_fmt(lblVer, "Suraksha+ Watch v%s", WATCH_OS_VERSION);

    // --- Restart button ---
    lv_obj_t* btnRestart = lv_btn_create(cont);
    lv_obj_set_size(btnRestart, 180, 32);
    Theme::styleDangerButton(btnRestart);
    lv_obj_t* lblRestart = lv_label_create(btnRestart);
    lv_label_set_text(lblRestart, "Restart Watch");
    lv_obj_center(lblRestart);
    lv_obj_add_event_cb(btnRestart, [](lv_event_t* e) {
        PowerManager::getInstance().restart();
    }, LV_EVENT_CLICKED, NULL);

    return scr;
}

void SettingsScreen::update(lv_obj_t* screen) {
    // Update brightness slider if changed externally
    if (sliderBrightness) {
        lv_slider_set_value(sliderBrightness,
            PowerManager::getInstance().getDisplayBrightness(), LV_ANIM_OFF);
    }
}
