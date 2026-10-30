#include "gesture_detector.h"
#include "../platform/power_manager.h"
#include "../drivers/vibration.h"
#include <Wire.h>

void GestureDetector::init() {
    _tapCount = 0;
    _pendingAction = 0;
    _touchActive = false;
    _hasSwiped = false;
    _waitingToFinalize = false;
    _initialized = true;

    // Initialize Touch Controller & Button pins
    calibrateTouch();
    Serial.println("[GEST] Gesture detector initialized");
}

void GestureDetector::calibrateTouch() {
    // Configure FT6336U I2C (Wire1: SDA=39, SCL=40)
#if defined(PIN_TOUCH_SDA) && defined(PIN_TOUCH_SCL)
    Wire1.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL);
    Wire1.setClock(400000);
    Wire1.setTimeOut(30);
    Serial.printf("[TOUCH] FT6336U I2C Bus initialized: SDA=%d, SCL=%d\n",
                  PIN_TOUCH_SDA, PIN_TOUCH_SCL);

    // Initialize FT6336U device mode
    Wire1.beginTransmission(0x38);
    Wire1.write(0x00); // DEV_MODE register
    Wire1.write(0x00); // Normal operating mode
    Wire1.endTransmission();
#endif

    // Configure touch interrupt pin (GPIO 16)
#if defined(PIN_TOUCH_INT) && PIN_TOUCH_INT >= 0
    pinMode(PIN_TOUCH_INT, INPUT_PULLUP);
    Serial.printf("[TOUCH] Touch interrupt pin configured: GPIO %d\n", PIN_TOUCH_INT);
#endif

    // Configure side physical button (GPIO 0 - active LOW)
    pinMode(0, INPUT_PULLUP);
    Serial.println("[TOUCH] Physical side button configured: GPIO 0");
}

void GestureDetector::update() {
    if (!_initialized) return;

    unsigned long now = millis();
    bool touchNow = false;
    int16_t screenX = -1;
    int16_t screenY = -1;

    // 1. Check FT6336U capacitive touch coordinates via Wire1 (I2C addr 0x38)
#if defined(PIN_TOUCH_INT) && PIN_TOUCH_INT >= 0
    bool intPinLow = (digitalRead(PIN_TOUCH_INT) == LOW);

    Wire1.beginTransmission(0x38);
    Wire1.write(0x00); // Read starting from register 0x00 (standard FocalTech bulk read)
    uint8_t err = Wire1.endTransmission();
    if (err == 0) {
        if (Wire1.requestFrom((uint8_t)0x38, (size_t)7) >= 7) {
            uint8_t devMode = Wire1.read();   // 0x00: DEV_MODE
            uint8_t gestId = Wire1.read();    // 0x01: GEST_ID
            uint8_t tdStatus = Wire1.read();  // 0x02: TD_STATUS (number of touches)
            uint8_t xh = Wire1.read();        // 0x03: Touch1 X High
            uint8_t xl = Wire1.read();        // 0x04: Touch1 X Low
            uint8_t yh = Wire1.read();        // 0x05: Touch1 Y High
            uint8_t yl = Wire1.read();        // 0x06: Touch1 Y Low

            uint8_t count = tdStatus & 0x0F;
            if ((count >= 1 && count <= 2) || intPinLow) {
                touchNow = true;
                uint16_t rawX = ((xh & 0x0F) << 8) | xl;
                uint16_t rawY = ((yh & 0x0F) << 8) | yl;
                // Rotation 2: invert coordinates (240x240 screen)
                screenX = (rawX < 240) ? (239 - rawX) : 120;
                screenY = (rawY < 240) ? (239 - rawY) : 120;
            }
        }
    } else {
        static uint32_t s_lastErrMs = 0;
        if (now - s_lastErrMs >= 5000) {
            s_lastErrMs = now;
            Serial.printf("[TOUCH] Wire1 transmission to 0x38 failed with code: %d\n", err);
        }
        if (intPinLow) {
            touchNow = true;
        }
    }
#endif

    // 2. Check physical side button (GPIO 0 - active LOW)
    bool isPressed = (digitalRead(0) == LOW);
    if (isPressed) {
        if (!_btnIsPressed) {
            // Just pressed down (transition from released to pressed)
            _btnIsPressed = true;
            _btnDownMs = now;
            _btnTriggeredLongPress = false;
        } else {
            // Held down: genuinely verify continuous 5-second hold before triggering
            if (!_btnTriggeredLongPress && (now - _btnDownMs >= GESTURE_LONG_PRESS_MS)) {
                _btnTriggeredLongPress = true;
                _pendingLongPress = true;
                _tapCount = 0;
                _waitingToFinalize = false;
                Vibration::getInstance().play(HapticPattern::SUCCESS);
                Serial.printf("[GEST] 🚨 PHYSICAL BUTTON 5s LONG PRESS (held %lums) -> CANCEL SOS\n", now - _btnDownMs);
            }
        }
    } else {
        if (_btnIsPressed) {
            // Just released (transition from pressed to released)
            _btnIsPressed = false;
            unsigned long pressDuration = now - _btnDownMs;

            // Short press (<600ms) counts as tap ONLY if SOS is NOT active
            if (!_btnTriggeredLongPress && (pressDuration >= 40) && (pressDuration < 600)) {
                if (!_sosActive) {
                    _tapCount++;
                    _lastTapEdgeMs = now;
                    _waitingToFinalize = true;
                    PowerManager::getInstance().resetInactivityTimer();
                    Vibration::getInstance().play(HapticPattern::TAP);
                    Serial.printf("[GEST] 🔘 Side Button TAP %d\n", _tapCount);
                }
            }
        }
    }

    // 3. Capacitive Touch State Machine (Swipes, Taps, and 5s Long Press)
    if (touchNow) {
        if (!_touchActive) {
            // Touch down edge
            _touchActive = true;
            _touchDownMs = now;
            _startX = (screenX >= 0) ? screenX : 120;
            _startY = (screenY >= 0) ? screenY : 120;
            _currentX = _startX;
            _currentY = _startY;
            _hasSwiped = false;
            _touchTriggeredLongPress = false;
            PowerManager::getInstance().resetInactivityTimer();
        } else {
            // Touch held / dragging
            if (screenX >= 0) {
                _currentX = screenX;
                _currentY = screenY;
            }
            int dx = _currentX - _startX;
            int dy = _currentY - _startY;

            // Check for 5-second long press hold
            if (!_touchTriggeredLongPress && (now - _touchDownMs >= GESTURE_LONG_PRESS_MS)) {
                _touchTriggeredLongPress = true;
                _pendingLongPress = true;
                _tapCount = 0;
                _waitingToFinalize = false;
                _hasSwiped = true; // Block tap on release
                Vibration::getInstance().play(HapticPattern::SUCCESS);
                Serial.println("[GEST] 🚨 TOUCH SCREEN 5.0s LONG PRESS -> CANCEL SOS");
            }

            // Swipes are ONLY allowed when SOS is NOT active!
            if (!_sosActive && !_hasSwiped && !_touchTriggeredLongPress) {
                // Generous swipe threshold: dx > 65px and predominantly horizontal
                if (dx < -65 && abs(dx) > (abs(dy) * 1.3)) {
                    _hasSwiped = true;
                    _pendingAction = GESTURE_ACTION_SWIPE_LEFT;
                    _tapCount = 0;
                    _waitingToFinalize = false;
                    Vibration::getInstance().play(HapticPattern::BUTTON_PRESS);
                    Serial.printf("[GEST] ➡️ SWIPE RIGHT-TO-LEFT (dx=%d) -> NEXT SCREEN\n", dx);
                } else if (dx > 65 && abs(dx) > (abs(dy) * 1.3)) {
                    _hasSwiped = true;
                    _pendingAction = GESTURE_ACTION_SWIPE_RIGHT;
                    _tapCount = 0;
                    _waitingToFinalize = false;
                    Vibration::getInstance().play(HapticPattern::BUTTON_PRESS);
                    Serial.printf("[GEST] ⬅️ SWIPE LEFT-TO-RIGHT (dx=%d) -> PREV SCREEN\n", dx);
                }
            }
        }
    } else {
        // Touch released
        if (_touchActive) {
            _touchActive = false;
            unsigned long touchDuration = now - _touchDownMs;

            // When SOS is active, TAPS MUST NEVER BE REGISTERED
            if (_sosActive) {
                _tapCount = 0;
                _waitingToFinalize = false;
            } else if (!_hasSwiped && !_touchTriggeredLongPress) {
                int totalMove = abs(_currentX - _startX) + abs(_currentY - _startY);
                // Permissive stationary tap detection (movement < 55px, duration 20..600ms)
                if (totalMove < 55 && touchDuration >= 20 && touchDuration < 600) {
                    _tapCount++;
                    _lastTapEdgeMs = now;
                    _waitingToFinalize = true;
                    Vibration::getInstance().play(HapticPattern::TAP);
                    Serial.printf("[GEST] 👆 TAP %d\n", _tapCount);
                }
            }
        }
    }

    // 4. Finalize multi-taps after idle period (500ms gap)
    if (_waitingToFinalize && !_touchActive && !isPressed &&
        (now - _lastTapEdgeMs >= GESTURE_TAP_IDLE_MS)) {
        _waitingToFinalize = false;

        // Discard any taps if SOS is active
        if (_sosActive) {
            _tapCount = 0;
            return;
        }

        Serial.printf("[GEST] Finalized: %d taps\n", _tapCount);

        if (_tapCount == 1) {
            _pendingAction = GESTURE_ACTION_NEXT_SCREEN; // 1 tap -> toggle/next screen
        } else if (_tapCount == 2) {
            _pendingAction = GESTURE_ACTION_SOS;         // 2 taps -> SOS
        } else if (_tapCount == 3) {
            _pendingAction = GESTURE_ACTION_ACT_OFF;     // 3 taps -> Activity OFF
        } else if (_tapCount >= 4) {
            _pendingAction = GESTURE_ACTION_ACT_ON;      // 4 taps -> Activity ON
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
