// ============================================================================
// Suraksha+ Watch OS — Touch Driver Implementation
// CST816S over I2C
// ============================================================================
#include "touch_driver.h"

// CST816S Register Map
#define CST816S_REG_GESTURE     0x01
#define CST816S_REG_FINGER_NUM  0x02
#define CST816S_REG_XPOS_H      0x03
#define CST816S_REG_XPOS_L      0x04
#define CST816S_REG_YPOS_H      0x05
#define CST816S_REG_YPOS_L      0x06
#define CST816S_REG_CHIP_ID     0xA7
#define CST816S_REG_SLEEP       0xA5
#define CST816S_REG_IRQ_CTL     0xFA
#define CST816S_REG_AUTO_RESET  0xFB
#define CST816S_REG_LONG_PRESS  0xFC
#define CST816S_REG_MOTION_MASK 0xEC

TouchDriver& TouchDriver::getInstance() {
    static TouchDriver instance;
    return instance;
}

void TouchDriver::init() {
    Serial.println("[TOUCH] Initializing CST816S...");

    // Setup interrupt pin
    if (PIN_TOUCH_INT >= 0) {
        pinMode(PIN_TOUCH_INT, INPUT);
    }

    // Reset if pin available
    if (PIN_TOUCH_RST >= 0) {
        pinMode(PIN_TOUCH_RST, OUTPUT);
        digitalWrite(PIN_TOUCH_RST, LOW);
        delay(10);
        digitalWrite(PIN_TOUCH_RST, HIGH);
        delay(50);
    }

    // Check chip ID
    uint8_t chipId = 0;
    if (_readRegisters(CST816S_REG_CHIP_ID, &chipId, 1)) {
        Serial.printf("[TOUCH] CST816S Chip ID: 0x%02X\n", chipId);
        _initialized = true;
    } else {
        Serial.println("[TOUCH] ⚠️ CST816S not found — touch disabled");
        _initialized = false;
    }

    if (_initialized) {
        // Enable motion detection for gestures
        _writeRegister(CST816S_REG_MOTION_MASK, 0x01); // Enable continuous reporting
        _writeRegister(CST816S_REG_IRQ_CTL, 0x60);     // IRQ on touch + motion
        _writeRegister(CST816S_REG_AUTO_RESET, 0x05);   // Auto reset after 5s
        _writeRegister(CST816S_REG_LONG_PRESS, 0x28);   // Long press time

        Serial.println("[TOUCH] ✅ CST816S initialized");
    }

    // Register LVGL input driver
    lv_indev_drv_init(&_indevDrv);
    _indevDrv.type = LV_INDEV_TYPE_POINTER;
    _indevDrv.read_cb = _lvglReadCb;
    _indevDrv.user_data = this;

    _lvIndev = lv_indev_drv_register(&_indevDrv);

    if (_lvIndev) {
        Serial.println("[TOUCH] LVGL input driver registered");
    }
}

TouchPoint TouchDriver::read() {
    if (!_initialized) {
        _lastPoint = {0, 0, false, TouchGesture::NONE};
        return _lastPoint;
    }

    uint8_t data[7];
    if (!_readRegisters(CST816S_REG_GESTURE, data, 7)) {
        _lastPoint.pressed = false;
        return _lastPoint;
    }

    // Parse gesture
    TouchGesture gesture = (TouchGesture)data[0];

    // Parse finger count
    uint8_t fingerCount = data[1];

    // Parse coordinates
    int16_t x = ((data[2] & 0x0F) << 8) | data[3];
    int16_t y = ((data[4] & 0x0F) << 8) | data[5];

    // Clamp to display bounds
    x = constrain(x, 0, DISPLAY_WIDTH - 1);
    y = constrain(y, 0, DISPLAY_HEIGHT - 1);

    _lastPoint.x = x;
    _lastPoint.y = y;
    _lastPoint.pressed = (fingerCount > 0);
    _lastPoint.gesture = gesture;

    // Fire gesture callback
    if (gesture != TouchGesture::NONE && _gestureCallback) {
        _gestureCallback(gesture);
    }

    return _lastPoint;
}

bool TouchDriver::_readRegisters(uint8_t reg, uint8_t* data, uint8_t len) {
    Wire.beginTransmission(TOUCH_I2C_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;

    uint8_t received = Wire.requestFrom((int)TOUCH_I2C_ADDR, (int)len);
    if (received != len) return false;

    for (uint8_t i = 0; i < len; i++) {
        data[i] = Wire.read();
    }
    return true;
}

bool TouchDriver::_writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(TOUCH_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    return (Wire.endTransmission() == 0);
}

void TouchDriver::_lvglReadCb(lv_indev_drv_t* indev, lv_indev_data_t* data) {
    TouchDriver* self = (TouchDriver*)indev->user_data;
    TouchPoint tp = self->read();

    if (tp.pressed) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = tp.x;
        data->point.y = tp.y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}
