// ============================================================================
// Suraksha+ Watch OS — Display Driver Implementation
// ST7789 240x240 TFT via TFT_eSPI — LilyGo T-Watch S3
// ============================================================================
#include "display_driver.h"

DisplayDriver& DisplayDriver::getInstance() {
    static DisplayDriver instance;
    return instance;
}

void DisplayDriver::init() {
    if (_poweredOn) return; // Prevent double initialization

    Serial.println("[DISP] Initializing ST7789 240x240 TFT via TFT_eSPI...");

    // Direct GPIO control for backlight to guarantee it stays ON
#if defined(PIN_TFT_BL) && PIN_TFT_BL >= 0
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);
#endif

    // Initialize TFT_eSPI
    _tft.init();
    _tft.setRotation(2); // 180° rotation — right-side up on T-Watch S3
    _tft.invertDisplay(true);

    // Initialize double-buffer sprite in PSRAM to eliminate all flickering
    _spr.setColorDepth(16);
    void* ptr = _spr.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (ptr) {
        _hasSprite = true;
        Serial.println("[DISP] ✅ Double-buffer PSRAM sprite created (240x240, 16-bit)");
    } else {
        _hasSprite = false;
        Serial.println("[DISP] ⚠️ Failed to create PSRAM sprite, falling back to direct TFT");
    }

    // Initial boot splash
    TFT_eSPI* canvas = getCanvas();
    canvas->fillScreen(TFT_NAVY);
    canvas->setTextColor(TFT_CYAN, TFT_NAVY);
    canvas->setTextDatum(MC_DATUM);
    canvas->drawString("SURAKSHA+ OS", 120, 100, 4);
    canvas->setTextColor(TFT_WHITE, TFT_NAVY);
    canvas->drawString("LilyGo T-Watch S3", 120, 140, 2);
    flush();

    _poweredOn = true;

    Serial.printf("[DISP] ✅ ST7789 TFT initialized: %dx%d (Rotation: 2, Sprite: %s)\n",
                  DISPLAY_WIDTH, DISPLAY_HEIGHT, _hasSprite ? "ENABLED" : "DISABLED");
}

TFT_eSPI* DisplayDriver::getCanvas() {
    return _hasSprite ? static_cast<TFT_eSPI*>(&_spr) : &_tft;
}

void DisplayDriver::flush() {
    if (_hasSprite) {
        _spr.pushSprite(0, 0);
    }
}

void DisplayDriver::powerOn() {
    if (!_poweredOn) {
#if defined(PIN_TFT_BL) && PIN_TFT_BL >= 0
        pinMode(PIN_TFT_BL, OUTPUT);
        digitalWrite(PIN_TFT_BL, HIGH);
#endif
        _tft.writecommand(0x29); // ST7789 display ON command
        _poweredOn = true;
        Serial.println("[DISP] TFT power ON");
    }
}

void DisplayDriver::powerOff() {
    if (_poweredOn) {
#if defined(PIN_TFT_BL) && PIN_TFT_BL >= 0
        pinMode(PIN_TFT_BL, OUTPUT);
        digitalWrite(PIN_TFT_BL, LOW);
#endif
        _tft.writecommand(0x28); // ST7789 display OFF command
        _poweredOn = false;
        Serial.println("[DISP] TFT power OFF");
    }
}

void DisplayDriver::setBrightness(uint8_t brightness) {
    _brightness = brightness;
#if defined(PIN_TFT_BL) && PIN_TFT_BL >= 0
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, (brightness > 0) ? HIGH : LOW);
#endif
}
