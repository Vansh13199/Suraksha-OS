// ============================================================================
// Suraksha+ Watch OS — Display Driver
// SH1106 128x64 OLED via U8g2 (I2C)
// ============================================================================
#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "../config/hardware_config.h"

class DisplayDriver {
public:
    static DisplayDriver& getInstance();

    void init();

    // Get the U8g2 instance for drawing
    U8G2_SH1106_128X64_NONAME_F_HW_I2C* getU8g2() { return &_u8g2; }

    // Display power control
    void powerOn();
    void powerOff();
    bool isPoweredOn() const { return _poweredOn; }

    // Contrast control (replaces backlight brightness)
    void setContrast(uint8_t contrast);
    uint8_t getContrast() const { return _contrast; }

private:
    DisplayDriver() : _u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE) {}

    U8G2_SH1106_128X64_NONAME_F_HW_I2C _u8g2;
    bool _poweredOn = false;
    uint8_t _contrast = OLED_DEFAULT_CONTRAST;
};

#endif // DISPLAY_DRIVER_H
