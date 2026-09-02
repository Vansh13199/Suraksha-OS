// ============================================================================
// Suraksha+ Watch OS — UI Manager
// U8g2-based screen management for SH1106 128x64 OLED
// ============================================================================
#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <U8g2lib.h>

enum class Screen {
    WATCHFACE,
    SOS_ACTIVE,
    HEALTH,
    SYSTEM_INFO
};

// Total number of auto-cycling screens (excludes SOS which is an overlay)
#define SCREEN_CYCLE_COUNT  3
#define SCREEN_CYCLE_MS     5000    // Auto-cycle every 5 seconds

class UIManager {
public:
    static UIManager& getInstance();

    void init();
    void update();  // Draw current screen to OLED

    // Screen navigation
    void showScreen(Screen screen);
    void nextScreen();
    Screen getCurrentScreen() const { return _currentScreen; }

    // SOS overlay — overrides normal screen cycle
    void showSOSOverlay(bool show);

    // Force refresh
    void refresh();

private:
    UIManager() = default;

    Screen _currentScreen = Screen::WATCHFACE;
    bool _sosOverride = false;
    bool _initialized = false;

    // Auto-cycle state
    uint32_t _lastCycleMs = 0;
    uint8_t _cycleIndex = 0;

    // Screen cycle order
    static const Screen _cycleScreens[SCREEN_CYCLE_COUNT];
};

#endif // UI_MANAGER_H
