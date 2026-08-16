// ============================================================================
// Suraksha+ Watch OS — UI Manager Implementation
// ============================================================================
#include "ui_manager.h"
#include "theme.h"
#include "screens/watchface_screen.h"
#include "screens/sos_screen.h"
#include "screens/health_screen.h"
#include "screens/apps_menu_screen.h"
#include "screens/settings_screen.h"
#include "../drivers/display_driver.h"
#include "../config/hardware_config.h"

UIManager& UIManager::getInstance() {
    static UIManager instance;
    return instance;
}

void UIManager::init() {
    Serial.println("[UI] Initializing UI manager...");

    // Initialize LVGL
    lv_init();
    Serial.println("[UI] LVGL initialized");

    // Initialize display driver (registers with LVGL)
    DisplayDriver::getInstance().init();

    // Apply theme
    lv_disp_t* disp = DisplayDriver::getInstance().getLvDisplay();
    if (disp) {
        Theme::applyDarkTheme(disp);
    }

    // Create initial screens
    _createScreens();

    // Show watchface first
    showScreen(Screen::WATCHFACE);

    _initialized = true;
    Serial.println("[UI] ✅ UI manager ready");
}

void UIManager::update() {
    if (!_initialized) return;
    lv_timer_handler();
}

void UIManager::showScreen(Screen screen) {
    _previousScreen = _currentScreen;
    _currentScreen = screen;

    lv_obj_t* target = nullptr;

    switch (screen) {
        case Screen::WATCHFACE:
            if (!_scrWatchface) {
                _scrWatchface = WatchfaceScreen::create();
            }
            WatchfaceScreen::update(_scrWatchface);
            target = _scrWatchface;
            break;

        case Screen::SOS_ACTIVE:
            if (!_scrSOS) {
                _scrSOS = SOSScreen::create();
            }
            SOSScreen::update(_scrSOS);
            target = _scrSOS;
            break;

        case Screen::HEALTH:
            if (!_scrHealth) {
                _scrHealth = HealthScreen::create();
            }
            HealthScreen::update(_scrHealth);
            target = _scrHealth;
            break;

        case Screen::APPS_MENU:
            if (!_scrApps) {
                _scrApps = AppsMenuScreen::create();
            }
            target = _scrApps;
            break;

        case Screen::SETTINGS:
            if (!_scrSettings) {
                _scrSettings = SettingsScreen::create();
            }
            target = _scrSettings;
            break;

        default:
            return;
    }

    if (target) {
        lv_scr_load_anim(target, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
    }
}

void UIManager::goBack() {
    showScreen(_previousScreen);
}

void UIManager::showSOSOverlay(bool show) {
    if (show) {
        showScreen(Screen::SOS_ACTIVE);
    } else {
        showScreen(Screen::WATCHFACE);
    }
}

void UIManager::showNotificationPopup(const char* title, const char* body) {
    // Create a temporary message box overlay
    lv_obj_t* mbox = lv_msgbox_create(NULL, title, body, NULL, true);
    lv_obj_center(mbox);
    lv_obj_set_style_bg_color(mbox, Theme::COLOR_SURFACE, 0);
    lv_obj_set_style_text_color(mbox, Theme::COLOR_TEXT, 0);

    // Auto-dismiss after 5 seconds
    lv_obj_add_event_cb(mbox, [](lv_event_t* e) {
        lv_obj_t* obj = lv_event_get_current_target(e);
        lv_msgbox_close(obj);
    }, LV_EVENT_CLICKED, NULL);
}

void UIManager::refresh() {
    lv_obj_invalidate(lv_scr_act());
}

void UIManager::_createScreens() {
    // Screens are lazily created on first show
    Serial.println("[UI] Screens will be created on demand");
}
