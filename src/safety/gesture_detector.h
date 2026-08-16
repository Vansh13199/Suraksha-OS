// ============================================================================
// Suraksha+ Watch OS — Gesture Detector
// Multi-tap and long-press detection (ported from final.ino)
// ============================================================================
#ifndef GESTURE_DETECTOR_H
#define GESTURE_DETECTOR_H

#include <Arduino.h>
#include "../config/app_config.h"
#include "../config/hardware_config.h"

class GestureDetector {
public:
    void init();
    void update();  // Call at ~66Hz from touch task

    // Feed touch input (from touch driver or capacitive pin)
    void feedTouch(bool isTouched);

    // Consume detected actions (returns 0 if no action)
    int consumeAction();
    bool consumeLongPress();

private:
    // Tap detection state (ported from final.ino handleTouchFixed)
    volatile int _tapCount = 0;
    volatile unsigned long _lastTapEdgeMs = 0;
    volatile unsigned long _touchDownMs = 0;
    volatile bool _waitingToFinalize = false;
    volatile int _pendingAction = 0;
    volatile bool _pendingLongPress = false;
    bool _currentlyTouched = false;

    // Debounce
    bool _lastIsTouch = false;
    unsigned long _lastStableTime = 0;

    bool _initialized = false;
};

#endif // GESTURE_DETECTOR_H
