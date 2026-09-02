// ============================================================================
// Suraksha+ Watch OS — UI Manager Implementation
// U8g2-based rendering for SH1106 128x64 OLED
// ============================================================================
#include "ui_manager.h"
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
    Serial.println("[UI] Initializing UI manager (U8g2)...");

    // Initialize display driver
    DisplayDriver::getInstance().init();

    _lastCycleMs = millis();
    _cycleIndex = 0;
    _currentScreen = Screen::WATCHFACE;
    _initialized = true;

    Serial.println("[UI] ✅ UI manager ready (SH1106 128x64)");
}

void UIManager::update() {
    if (!_initialized) return;

    U8G2_SH1106_128X64_NONAME_F_HW_I2C* u8g2 = DisplayDriver::getInstance().getU8g2();
    if (!u8g2) return;

    // Auto-cycle screens (unless SOS is active)
    if (!_sosOverride) {
        uint32_t now = millis();
        if (now - _lastCycleMs >= SCREEN_CYCLE_MS) {
            _lastCycleMs = now;
            _cycleIndex = (_cycleIndex + 1) % SCREEN_CYCLE_COUNT;
            _currentScreen = _cycleScreens[_cycleIndex];
        }
    }

    // Clear buffer
    u8g2->clearBuffer();

    // Draw the current screen
    switch (_currentScreen) {
        case Screen::WATCHFACE:
            WatchfaceScreen::draw(u8g2);
            break;

        case Screen::SOS_ACTIVE:
            SOSScreen::draw(u8g2);
            break;

        case Screen::HEALTH:
            HealthScreen::draw(u8g2);
            break;

        case Screen::SYSTEM_INFO:
            SystemInfoScreen::draw(u8g2);
            break;
    }

    // Send buffer to OLED
    u8g2->sendBuffer();
}

void UIManager::showScreen(Screen screen) {
    _currentScreen = screen;
    _lastCycleMs = millis(); // Reset cycle timer
}

void UIManager::nextScreen() {
    if (!_sosOverride) {
        _cycleIndex = (_cycleIndex + 1) % SCREEN_CYCLE_COUNT;
        _currentScreen = _cycleScreens[_cycleIndex];
        _lastCycleMs = millis();
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
