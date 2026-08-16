// ============================================================================
// Suraksha+ Watch OS — Power Manager Implementation
// ============================================================================
#include "power_manager.h"
#include "../config/hardware_config.h"
#include "../config/app_config.h"
#include <Preferences.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>

PowerManager& PowerManager::getInstance() {
    static PowerManager instance;
    return instance;
}

void PowerManager::init() {
    // Setup backlight PWM
    ledcSetup(DISPLAY_BL_CHANNEL, DISPLAY_BL_FREQ, DISPLAY_BL_RESOLUTION);
    ledcAttachPin(PIN_DISPLAY_BL, DISPLAY_BL_CHANNEL);

    // Load saved brightness
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    _brightness = prefs.getUChar(NVS_KEY_BRIGHTNESS, DISPLAY_BL_DEFAULT);
    _screenTimeoutMs = prefs.getUInt(NVS_KEY_SCREEN_TIMEOUT, SCREEN_TIMEOUT_DEFAULT_MS);
    _powerSave = prefs.getBool(NVS_KEY_POWER_SAVE, false);
    prefs.end();

    setDisplayBrightness(_brightness);
    _lastActivityMs = millis();
    _detectWakeReason();

    Serial.printf("[PWR] Init OK | Brightness: %d | Timeout: %dms | PowerSave: %d\n",
                  _brightness, _screenTimeoutMs, _powerSave);
}

void PowerManager::update() {
    if (_sleepPrevented) {
        _state = PowerState::ACTIVE;
        return;
    }

    uint32_t now = millis();
    uint32_t elapsed = now - _lastActivityMs;

    switch (_state) {
        case PowerState::ACTIVE:
            if (elapsed >= _screenTimeoutMs - SCREEN_DIM_TIMEOUT_MS && _displayOn) {
                _dimDisplay();
                _state = PowerState::DIM;
                _dimStartMs = now;
            }
            break;

        case PowerState::DIM:
            if (elapsed >= _screenTimeoutMs) {
                displayOff();
                _state = PowerState::SCREEN_OFF;
            }
            break;

        case PowerState::SCREEN_OFF:
            // Stay in screen-off; could transition to light sleep later
            // For now, we keep the MCU active for BLE and sensors
            break;

        case PowerState::LIGHT_SLEEP:
        case PowerState::DEEP_SLEEP:
            // These states are entered explicitly
            break;
    }
}

void PowerManager::setState(PowerState state) {
    if (_state == state) return;

    switch (state) {
        case PowerState::ACTIVE:
            displayOn();
            setDisplayBrightness(_savedBrightness);
            break;
        case PowerState::DIM:
            _dimDisplay();
            break;
        case PowerState::SCREEN_OFF:
            displayOff();
            break;
        case PowerState::LIGHT_SLEEP:
            enterLightSleep();
            return; // CPU is sleeping
        case PowerState::DEEP_SLEEP:
            enterDeepSleep();
            return; // CPU is sleeping
    }
    _state = state;
}

void PowerManager::resetInactivityTimer() {
    _lastActivityMs = millis();

    // Wake up if screen was off or dimmed
    if (_state != PowerState::ACTIVE) {
        displayOn();
        setDisplayBrightness(_savedBrightness);
        _state = PowerState::ACTIVE;
    }
}

void PowerManager::preventSleep(bool prevent) {
    _sleepPrevented = prevent;
    if (prevent && !_displayOn) {
        displayOn();
        setDisplayBrightness(_savedBrightness);
        _state = PowerState::ACTIVE;
    }
    Serial.printf("[PWR] Sleep prevention: %s\n", prevent ? "ON" : "OFF");
}

void PowerManager::setDisplayBrightness(uint8_t brightness) {
    _brightness = constrain(brightness, DISPLAY_BL_MIN, DISPLAY_BL_MAX);
    _savedBrightness = _brightness;

    if (_displayOn) {
        ledcWrite(DISPLAY_BL_CHANNEL, _brightness);
    }

    // Save to NVS
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar(NVS_KEY_BRIGHTNESS, _brightness);
    prefs.end();
}

void PowerManager::displayOn() {
    if (!_displayOn) {
        ledcWrite(DISPLAY_BL_CHANNEL, _brightness);
        _displayOn = true;
        Serial.println("[PWR] Display ON");
    }
}

void PowerManager::displayOff() {
    if (_displayOn) {
        ledcWrite(DISPLAY_BL_CHANNEL, 0);
        _displayOn = false;
        Serial.println("[PWR] Display OFF");
    }
}

void PowerManager::setPowerSaveMode(bool enabled) {
    _powerSave = enabled;

    if (enabled) {
        // Reduce brightness
        if (_brightness > 100) {
            setDisplayBrightness(100);
        }
        // Reduce screen timeout
        _screenTimeoutMs = 5000;
    } else {
        // Restore settings
        Preferences prefs;
        prefs.begin(NVS_NAMESPACE, true);
        _brightness = prefs.getUChar(NVS_KEY_BRIGHTNESS, DISPLAY_BL_DEFAULT);
        _screenTimeoutMs = prefs.getUInt(NVS_KEY_SCREEN_TIMEOUT, SCREEN_TIMEOUT_DEFAULT_MS);
        prefs.end();
        setDisplayBrightness(_brightness);
    }

    // Save power save state
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBool(NVS_KEY_POWER_SAVE, _powerSave);
    prefs.end();

    Serial.printf("[PWR] Power save: %s\n", enabled ? "ON" : "OFF");
}

void PowerManager::setScreenTimeout(uint32_t timeoutMs) {
    _screenTimeoutMs = timeoutMs;
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUInt(NVS_KEY_SCREEN_TIMEOUT, timeoutMs);
    prefs.end();
}

void PowerManager::enterLightSleep() {
    Serial.println("[PWR] Entering light sleep...");
    displayOff();
    _state = PowerState::LIGHT_SLEEP;

    // Configure wake sources
    _setupWakeSources();

    // Enter light sleep
    esp_light_sleep_start();

    // Woke up!
    _detectWakeReason();
    _lastActivityMs = millis();
    displayOn();
    _state = PowerState::ACTIVE;
    Serial.println("[PWR] Woke from light sleep");
}

void PowerManager::enterDeepSleep() {
    Serial.println("[PWR] Entering deep sleep...");
    displayOff();
    _state = PowerState::DEEP_SLEEP;

    // Configure wake sources
    _setupWakeSources();

    // Save crash-safe flag
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBool("clean_sleep", true);
    prefs.end();

    esp_deep_sleep_start();
    // Never reaches here
}

void PowerManager::restart() {
    Serial.println("[PWR] Restarting...");
    ESP.restart();
}

void PowerManager::shutdown() {
    Serial.println("[PWR] Shutting down...");
    displayOff();
    // Enter deep sleep with no wake sources = effective shutdown
    // Only USB plug-in or reset button will wake it
    esp_deep_sleep_start();
}

void PowerManager::_setupWakeSources() {
    // Touch pin as wake source
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BOOT_BUTTON, 0); // LOW = pressed

    // Timer wake (periodic sensor check)
    esp_sleep_enable_timer_wakeup(60 * 1000000ULL); // 60 seconds
}

void PowerManager::_dimDisplay() {
    uint8_t dimBrightness = _brightness / 3;
    if (dimBrightness < DISPLAY_BL_MIN) dimBrightness = DISPLAY_BL_MIN;
    ledcWrite(DISPLAY_BL_CHANNEL, dimBrightness);
}

void PowerManager::_detectWakeReason() {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    switch (cause) {
        case ESP_SLEEP_WAKEUP_EXT0:
            _wakeReason = WakeReason::BUTTON;
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            _wakeReason = WakeReason::TOUCH;
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            _wakeReason = WakeReason::TIMER;
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            _wakeReason = WakeReason::TOUCH;
            break;
        default:
            _wakeReason = WakeReason::BOOT;
            break;
    }
}
