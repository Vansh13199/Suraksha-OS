// ============================================================================
// Suraksha+ Watch OS — Gesture Detector
// Multi-tap and long-press detection (ported from final.ino)
// ============================================================================
#ifndef GESTURE_DETECTOR_H
#define GESTURE_DETECTOR_H

#include <Arduino.h>
#include "../config/app_config.h"
#include "../config/hardware_config.h"

#define GESTURE_ACTION_NONE         0
#define GESTURE_ACTION_NEXT_SCREEN  1
#define GESTURE_ACTION_SOS          2
#define GESTURE_ACTION_ACT_OFF      3
#define GESTURE_ACTION_ACT_ON       4
#define GESTURE_ACTION_SWIPE_LEFT   10  // Swipe Right-to-Left -> Next Screen
#define GESTURE_ACTION_SWIPE_RIGHT  11  // Swipe Left-to-Right -> Prev Screen

class GestureDetector {
public:
    void init();
    void update();  // Call at ~50Hz from safety task
    void calibrateTouch();

    // Consume detected actions (returns 0 if no action)
    int consumeAction();
    bool consumeLongPress();

    // Live tap feedback for UI
    int getTapCount() const { return _tapCount; }
    bool isWaitingToFinalize() const { return _waitingToFinalize; }

    // Inform gesture detector if SOS is active (blocks all taps/swipes except 5s long press)
    void setSOSActive(bool active) {
        if (_sosActive == active) return;
        _sosActive = active;
        _btnIsPressed = false;
        _btnTriggeredLongPress = false;
        _btnDownMs = millis();
        _touchActive = false;
        _touchTriggeredLongPress = false;
        _touchDownMs = millis();
        _tapCount = 0;
        _waitingToFinalize = false;
    }
    bool isSOSActive() const { return _sosActive; }

private:
    // Tap detection state
    volatile int _tapCount = 0;
    volatile unsigned long _lastTapEdgeMs = 0;
    volatile unsigned long _touchDownMs = 0;
    volatile bool _waitingToFinalize = false;
    volatile int _pendingAction = 0;
    volatile bool _pendingLongPress = false;

    // Coordinate & swipe tracking
    bool _touchActive = false;
    int16_t _startX = 0;
    int16_t _startY = 0;
    int16_t _currentX = 0;
    int16_t _currentY = 0;
    bool _hasSwiped = false;
    bool _touchTriggeredLongPress = false;

    // Side physical button tracking
    bool _btnIsPressed = false;
    unsigned long _btnDownMs = 0;
    bool _btnTriggeredLongPress = false;

    bool _sosActive = false;
    bool _initialized = false;
};

#endif // GESTURE_DETECTOR_H
