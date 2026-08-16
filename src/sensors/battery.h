// ============================================================================
// Suraksha+ Watch OS — Battery Sensor
// ============================================================================
#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>
#include "../config/hardware_config.h"
#include "../config/app_config.h"

struct BatteryData {
    uint16_t voltage_mV;    // Raw voltage in millivolts
    uint8_t percentage;     // 0-100%
    bool isCharging;        // USB connected / charging
    bool isLow;             // Below low threshold
    bool isCritical;        // Below critical threshold
    uint32_t timestamp;
};

class BatterySensor {
public:
    bool init();
    void update();
    BatteryData getData() const { return _data; }
    uint8_t getPercentage() const { return _data.percentage; }
    bool isCharging() const { return _data.isCharging; }
    bool isCritical() const { return _data.isCritical; }

private:
    uint16_t _readVoltage();
    uint8_t _voltageToPercent(uint16_t mv);

    BatteryData _data = {};
    bool _initialized = false;

    // Smoothing
    static const int AVG_SAMPLES = 8;
    uint16_t _voltageBuf[AVG_SAMPLES] = {};
    int _bufIdx = 0;
    int _bufCount = 0;
};

#endif // BATTERY_H
