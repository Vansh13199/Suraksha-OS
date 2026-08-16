// ============================================================================
// Suraksha+ Watch OS — Touch Driver
// CST816S capacitive touch controller (Seeed Round Display)
// ============================================================================
#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H

#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include "../config/hardware_config.h"

// CST816S gesture types
enum class TouchGesture : uint8_t {
    NONE        = 0x00,
    SWIPE_UP    = 0x01,
    SWIPE_DOWN  = 0x02,
    SWIPE_LEFT  = 0x03,
    SWIPE_RIGHT = 0x04,
    SINGLE_TAP  = 0x05,
    DOUBLE_TAP  = 0x0B,
    LONG_PRESS  = 0x0C
};

// Touch point data
struct TouchPoint {
    int16_t x;
    int16_t y;
    bool pressed;
    TouchGesture gesture;
};

// Gesture callback type
typedef void (*GestureCallback)(TouchGesture gesture);

class TouchDriver {
public:
    static TouchDriver& getInstance();

    void init();

    // Get latest touch data
    TouchPoint read();
    bool isTouched() const { return _lastPoint.pressed; }
    TouchPoint getLastPoint() const { return _lastPoint; }

    // Gesture callbacks
    void onGesture(GestureCallback callback) { _gestureCallback = callback; }

    // LVGL input driver
    lv_indev_t* getLvInput() { return _lvIndev; }

private:
    TouchDriver() = default;

    // CST816S I2C communication
    bool _readRegisters(uint8_t reg, uint8_t* data, uint8_t len);
    bool _writeRegister(uint8_t reg, uint8_t value);

    // LVGL read callback (static for C API)
    static void _lvglReadCb(lv_indev_drv_t* indev, lv_indev_data_t* data);

    TouchPoint _lastPoint = {0, 0, false, TouchGesture::NONE};
    GestureCallback _gestureCallback = nullptr;
    lv_indev_t* _lvIndev = nullptr;
    lv_indev_drv_t _indevDrv;

    bool _initialized = false;
};

#endif // TOUCH_DRIVER_H
