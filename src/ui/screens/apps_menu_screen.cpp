// ============================================================================
// Suraksha+ Watch OS — Apps Menu Screen Implementation
// ============================================================================
#include "apps_menu_screen.h"
#include "../theme.h"
#include "../ui_manager.h"

struct AppItem {
    const char* icon;
    const char* name;
    Screen screen;
};

static const AppItem APPS[] = {
    { LV_SYMBOL_WARNING,    "SOS",          Screen::SOS_ACTIVE },
    { LV_SYMBOL_WARNING,    "Health",       Screen::HEALTH },
    { LV_SYMBOL_SETTINGS,   "Settings",     Screen::SETTINGS },
    { LV_SYMBOL_BELL,       "Alarms",       Screen::ALARM },
    { LV_SYMBOL_REFRESH,    "Stopwatch",    Screen::STOPWATCH },
    { LV_SYMBOL_LOOP,       "Timer",        Screen::TIMER },
    { LV_SYMBOL_CALL,       "Find Phone",   Screen::FIND_PHONE },
    { LV_SYMBOL_CHARGE,     "Battery",      Screen::BATTERY },
    { LV_SYMBOL_BLUETOOTH,  "BLE",          Screen::BLE_STATUS },
};

static void _appBtnCb(lv_event_t* e) {
    Screen* target = (Screen*)lv_event_get_user_data(e);
    UIManager::getInstance().showScreen(*target);
}

lv_obj_t* AppsMenuScreen::create() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, Theme::COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Title
    lv_obj_t* title = lv_label_create(scr);
    lv_obj_set_style_text_font(title, Theme::FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title, Theme::COLOR_TEXT, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_label_set_text(title, "Apps");

    // Scrollable list container
    lv_obj_t* list = lv_obj_create(scr);
    lv_obj_set_size(list, 200, 170);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(list, 4, 0);

    int numApps = sizeof(APPS) / sizeof(APPS[0]);
    // Static storage for Screen values
    static Screen appScreens[sizeof(APPS) / sizeof(APPS[0])];

    for (int i = 0; i < numApps; i++) {
        appScreens[i] = APPS[i].screen;

        lv_obj_t* btn = lv_btn_create(list);
        lv_obj_set_size(btn, 190, 36);
        lv_obj_set_style_bg_color(btn, Theme::COLOR_SURFACE, 0);
        lv_obj_set_style_radius(btn, Theme::RADIUS, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_bg_color(btn, Theme::COLOR_SURFACE_LIGHT, LV_STATE_PRESSED);
        lv_obj_add_event_cb(btn, _appBtnCb, LV_EVENT_CLICKED, &appScreens[i]);

        // Icon + Label
        lv_obj_t* icon = lv_label_create(btn);
        lv_obj_set_style_text_font(icon, Theme::FONT_MEDIUM, 0);
        lv_obj_set_style_text_color(icon, Theme::COLOR_PRIMARY, 0);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 5, 0);
        lv_label_set_text(icon, APPS[i].icon);

        lv_obj_t* label = lv_label_create(btn);
        lv_obj_set_style_text_font(label, Theme::FONT_NORMAL, 0);
        lv_obj_set_style_text_color(label, Theme::COLOR_TEXT, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 30, 0);
        lv_label_set_text(label, APPS[i].name);
    }

    return scr;
}

void AppsMenuScreen::update(lv_obj_t* screen) {
    // Static screen — no dynamic updates needed
}
