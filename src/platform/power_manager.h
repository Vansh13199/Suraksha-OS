// ============================================================================
// Suraksha+ Watch OS — Power Manager
// Manages display sleep, light sleep, deep sleep, and wake sources
// ============================================================================
#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>

// Power states
enum class PowerState {
    ACTIVE,         // Full power, display on, all sensors active
    DIM,            // Display dimmed, sensors active
    SCREEN_OFF,     // Display off, sensors reduced rate
    LIGHT_SLEEP,    // CPU light sleep, wake on interrupt
    DEEP_SLEEP      // Deep sleep, wake on touch/button/timer
};

// Wake reasons
enum class WakeReason {
    BOOT,           // Initial boot
    TOUCH,          // Touch screen tap
    BUTTON,         // Boot button press
    BLE_EVENT,      // BLE connection/data
    TIMER,          // RTC timer
    ACCEL_TAP,      // Accelerometer tap interrupt
    SOS_ACTIVE,     // SOS is active (prevent sleep)
    UNKNOWN
};

class PowerManager {
public:
    static PowerManager& getInstance();

    void init();
    void update();                              // Call in main loop

    // State management
    PowerState getState() const { return _state; }
    void setState(PowerState state);
    void resetInactivityTimer();                // Call on any user interaction
    void preventSleep(bool prevent);            // Lock awake (during SOS)

    // Display control
    void setDisplayBrightness(uint8_t brightness);
    uint8_t getDisplayBrightness() const { return _brightness; }
    void displayOn();
    void displayOff();
    bool isDisplayOn() const { return _displayOn; }

    // Power saving
    void setPowerSaveMode(bool enabled);
    bool isPowerSaveMode() const { return _powerSave; }

    // Screen timeout
    void setScreenTimeout(uint32_t timeoutMs);
    uint32_t getScreenTimeout() const { return _screenTimeoutMs; }

    // Sleep
    void enterLightSleep();
    void enterDeepSleep();
    WakeReason getWakeReason() const { return _wakeReason; }

    // Shutdown
    void restart();
    void shutdown();

private:
    PowerManager() = default;
    PowerManager(const PowerManager&) = delete;
    PowerManager& operator=(const PowerManager&) = delete;

    void _setupWakeSources();
    void _dimDisplay();
    void _detectWakeReason();

    PowerState  _state = PowerState::ACTIVE;
    WakeReason  _wakeReason = WakeReason::BOOT;
    uint8_t     _brightness = 180;
    uint8_t     _savedBrightness = 180;
    bool        _displayOn = true;
    bool        _powerSave = false;
    bool        _sleepPrevented = false;
    uint32_t    _screenTimeoutMs = 10000;
    uint32_t    _lastActivityMs = 0;
    uint32_t    _dimStartMs = 0;
};

#endif // POWER_MANAGER_H
