// ============================================================================
// Suraksha+ Watch OS — Display Driver
// ST7789 240x240 TFT via TFT_eSPI — LilyGo T-Watch S3
// Double-buffered with PSRAM sprite for zero-flicker rendering
// ============================================================================
#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "../config/hardware_config.h"

class DisplayDriver {
public:
    static DisplayDriver& getInstance();

    void init();

    // Get canvas for zero-flicker drawing (double-buffered sprite or TFT fallback)
    TFT_eSPI* getCanvas();
    TFT_eSprite* getSprite() { return &_spr; }
    TFT_eSPI* getTFT() { return &_tft; }

    // Push the double-buffered frame to the screen
    void flush();

    // Display power control
    void powerOn();
    void powerOff();
    bool isPoweredOn() const { return _poweredOn; }

    // Brightness control
    void setBrightness(uint8_t brightness);
    uint8_t getBrightness() const { return _brightness; }

    void setContrast(uint8_t contrast) { setBrightness(contrast); }
    uint8_t getContrast() const { return _brightness; }

private:
    DisplayDriver() : _spr(&_tft) {}

    TFT_eSPI _tft;
    TFT_eSprite _spr;
    bool _hasSprite = false;
    bool _poweredOn = false;
    uint8_t _brightness = 200;
};

#endif // DISPLAY_DRIVER_H
