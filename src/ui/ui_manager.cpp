// ============================================================================
// Suraksha+ Watch OS — UI Manager Implementation
// TFT_eSPI-based rendering for LilyGo T-Watch S3 (240x240 ST7789)
// ============================================================================
#include "ui_manager.h"
#include "../config/hardware_config.h"
#include "../platform/power_manager.h"
#include "screens/watchface_screen.h"
#include "screens/sos_screen.h"
#include "screens/health_screen.h"
#include "screens/system_info_screen.h"
#include "../drivers/display_driver.h"

// Auto-cycle screen order
const Screen UIManager::_cycleScreens[SCREEN_CYCLE_COUNT] = {
    Screen::WATCHFACE,
    Screen::HEALTH,
    Screen::SYSTEM_INFO
};

UIManager& UIManager::getInstance() {
    static UIManager instance;
    return instance;
}

void UIManager::init() {
    Serial.println("[UI] Initializing UI manager (TFT_eSPI)...");

    // Initialize display driver
    DisplayDriver::getInstance().init();

    _lastCycleMs = millis();
    _cycleIndex = 0;
    _currentScreen = Screen::WATCHFACE;
    _initialized = true;

    Serial.println("[UI] ✅ UI manager ready (ST7789 240x240)");
}

void UIManager::update() {
    if (!_initialized) return;

    DisplayDriver& disp = DisplayDriver::getInstance();
    TFT_eSPI* canvas = disp.getCanvas();
    if (!canvas) return;

    // Screens stay on current screen unless swiped or triggered (No auto-cycling)

    // Draw the current screen into offscreen PSRAM canvas
    switch (_currentScreen) {
        case Screen::WATCHFACE:
            WatchfaceScreen::draw(canvas);
            break;

        case Screen::SOS_ACTIVE:
            SOSScreen::draw(canvas);
            break;

        case Screen::HEALTH:
            HealthScreen::draw(canvas);
            break;

        case Screen::SYSTEM_INFO:
            SystemInfoScreen::draw(canvas);
            break;
    }

    // Push frame to physical TFT in one single transfer (eliminates all flickering)
    disp.flush();
}

void UIManager::showScreen(Screen screen) {
    _currentScreen = screen;
}

void UIManager::nextScreen() {
    if (!_sosOverride) {
        _cycleIndex = (_cycleIndex + 1) % SCREEN_CYCLE_COUNT;
        _currentScreen = _cycleScreens[_cycleIndex];
        Serial.printf("[UI] ➡️ Swiped to next screen: %d\n", (int)_currentScreen);
    }
}

void UIManager::prevScreen() {
    if (!_sosOverride) {
        _cycleIndex = (_cycleIndex + SCREEN_CYCLE_COUNT - 1) % SCREEN_CYCLE_COUNT;
        _currentScreen = _cycleScreens[_cycleIndex];
        Serial.printf("[UI] ⬅️ Swiped to previous screen: %d\n", (int)_currentScreen);
    }
}

void UIManager::showSOSOverlay(bool show) {
    _sosOverride = show;
    if (show) {
        _currentScreen = Screen::SOS_ACTIVE;
    } else {
        _currentScreen = Screen::WATCHFACE;
        _lastCycleMs = millis();
    }
}

void UIManager::refresh() {
    update();
}
