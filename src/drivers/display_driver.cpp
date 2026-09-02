// ============================================================================
// Suraksha+ Watch OS — Display Driver Implementation
// SH1106 128x64 OLED via U8g2 (I2C)
// ============================================================================
#include "display_driver.h"

DisplayDriver& DisplayDriver::getInstance() {
    static DisplayDriver instance;
    return instance;
}

void DisplayDriver::init() {
    Serial.println("[DISP] Initializing SH1106 128x64 OLED...");
    Serial.printf("[DISP] I2C pins: SDA=%d, SCL=%d, Addr=0x%02X\n",
                  PIN_SENSOR_SDA, PIN_SENSOR_SCL, OLED_I2C_ADDR);

    // Use the Wire instance that was already initialized with correct pins
    // U8g2 begin() would call Wire.begin() with default pins, so we use
    // setBusClock + begin explicitly to keep our pin configuration
    _u8g2.setI2CAddress(OLED_I2C_ADDR * 2); // U8g2 uses 8-bit I2C address
    _u8g2.begin();

    // Re-initialize Wire with our pins since U8g2 begin() may override them
    Wire.begin(PIN_SENSOR_SDA, PIN_SENSOR_SCL);
    Wire.setClock(I2C_SPEED);

    // Set default contrast
    _u8g2.setContrast(_contrast);

    // Clear and show a brief splash
    _u8g2.clearBuffer();
    _u8g2.setFont(u8g2_font_6x10_tr);
    _u8g2.drawStr(18, 28, "SURAKSHA+ OS");
    _u8g2.drawStr(30, 42, "v" "1.0.0");
    _u8g2.sendBuffer();

    _poweredOn = true;

    Serial.printf("[DISP] ✅ SH1106 OLED initialized: %dx%d\n",
                  DISPLAY_WIDTH, DISPLAY_HEIGHT);
}

void DisplayDriver::powerOn() {
    if (!_poweredOn) {
        _u8g2.setPowerSave(0);
        _poweredOn = true;
        Serial.println("[DISP] OLED power ON");
    }
}

void DisplayDriver::powerOff() {
    if (_poweredOn) {
        _u8g2.setPowerSave(1);
        _poweredOn = false;
        Serial.println("[DISP] OLED power OFF");
    }
}

void DisplayDriver::setContrast(uint8_t contrast) {
    _contrast = contrast;
    _u8g2.setContrast(contrast);
}
