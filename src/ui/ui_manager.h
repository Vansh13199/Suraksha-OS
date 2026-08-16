// ============================================================================
// Suraksha+ Watch OS — UI Manager
// LVGL initialization, screen management, and navigation
// ============================================================================
#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <lvgl.h>

enum class Screen {
    WATCHFACE,
    SOS_ACTIVE,
    HEALTH,
    APPS_MENU,
    SETTINGS,
    NOTIFICATIONS,
    BATTERY,
    BLE_STATUS,
    ACTIVITY,
    AI_ASSISTANT,
    STOPWATCH,
    TIMER,
    ALARM,
    FIND_PHONE
};

class UIManager {
public:
    static UIManager& getInstance();

    void init();
    void update();  // Call lv_timer_handler

    // Screen navigation
    void showScreen(Screen screen);
    void goBack();
    Screen getCurrentScreen() const { return _currentScreen; }

    // SOS overlay
    void showSOSOverlay(bool show);

    // Notifications
    void showNotificationPopup(const char* title, const char* body);

    // Force refresh
    void refresh();

private:
    UIManager() = default;

    void _createScreens();

    Screen _currentScreen = Screen::WATCHFACE;
    Screen _previousScreen = Screen::WATCHFACE;

    // Screen objects
    lv_obj_t* _scrWatchface = nullptr;
    lv_obj_t* _scrSOS = nullptr;
    lv_obj_t* _scrHealth = nullptr;
    lv_obj_t* _scrApps = nullptr;
    lv_obj_t* _scrSettings = nullptr;
    lv_obj_t* _scrNotifications = nullptr;

    bool _initialized = false;
};

#endif // UI_MANAGER_H
