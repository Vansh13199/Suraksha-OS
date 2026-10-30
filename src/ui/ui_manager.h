// ============================================================================
// Suraksha+ Watch OS — UI Manager
// TFT_eSPI-based screen management for LilyGo T-Watch S3 (240x240 ST7789)
// ============================================================================
#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>

enum class Screen {
    WATCHFACE,
    SOS_ACTIVE,
    HEALTH,
    SYSTEM_INFO
};

// Total number of auto-cycling screens (excludes SOS which is an overlay)
#define SCREEN_CYCLE_COUNT  3
#define SCREEN_CYCLE_MS     6000    // Auto-cycle screens every 6 seconds

class UIManager {
public:
    static UIManager& getInstance();

    void init();
    void update();  // Draw current screen to TFT

    // Screen navigation (Swipe left / right)
    void showScreen(Screen screen);
    void nextScreen();
    void prevScreen();
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
