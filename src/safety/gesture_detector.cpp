// ============================================================================
// Suraksha+ Watch OS — Gesture Detector Implementation
// Ported directly from final.ino handleTouchFixed() logic
// ============================================================================
#include "gesture_detector.h"

void GestureDetector::init() {
    _tapCount = 0;
    _pendingAction = 0;
    _pendingLongPress = false;
    _currentlyTouched = false;
    _waitingToFinalize = false;
    _initialized = true;
    Serial.println("[GEST] Gesture detector initialized");
}

void GestureDetector::update() {
    // This is called from the touch task
    // Touch input is fed via feedTouch()
}

void GestureDetector::feedTouch(bool isTouch) {
    unsigned long now = millis();

    // Debounce: only act if state is stable for GESTURE_DEBOUNCE_MS
    if (isTouch != _lastIsTouch) {
        _lastStableTime = now;
        _lastIsTouch = isTouch;
    }

    if (now - _lastStableTime < GESTURE_DEBOUNCE_MS) return;

    if (isTouch) {
        if (!_currentlyTouched) {
            // New touch down
            _currentlyTouched = true;
            _touchDownMs = now;
            _tapCount++;
            _lastTapEdgeMs = now;
            _waitingToFinalize = true;
            Serial.printf("[GEST] TAP %d\n", _tapCount);
        }

        // Check for long press (5 seconds)
        if (_currentlyTouched) {
            unsigned long heldDuration = now - _touchDownMs;
            if (heldDuration >= GESTURE_LONG_PRESS_MS) {
                Serial.println("[GEST] 🚨 LONG PRESS detected (5s)");
                _pendingLongPress = true;
                _currentlyTouched = false;
                _tapCount = 0;
                _waitingToFinalize = false;
                _pendingAction = 0;
                return;
            }
        }
    } else {
        if (_currentlyTouched) {
            // Touch released
            _currentlyTouched = false;
            _lastTapEdgeMs = now;
        }
    }

    // Finalize tap count after idle period
    if (_waitingToFinalize && !_currentlyTouched &&
        (now - _lastTapEdgeMs >= GESTURE_TAP_IDLE_MS)) {
        _waitingToFinalize = false;

        Serial.printf("[GEST] Finalized: %d taps\n", _tapCount);

        // Map tap count to action (same as existing firmware)
        if (_tapCount == 2) {
            _pendingAction = 2;      // SOS
        } else if (_tapCount == 3) {
            _pendingAction = 3;      // Activity OFF
        } else if (_tapCount >= 4) {
            _pendingAction = 4;      // Activity ON
        }

        _tapCount = 0;
    }
}

int GestureDetector::consumeAction() {
    int action = _pendingAction;
    _pendingAction = 0;
    return action;
}

bool GestureDetector::consumeLongPress() {
    bool lp = _pendingLongPress;
    _pendingLongPress = false;
    return lp;
}
