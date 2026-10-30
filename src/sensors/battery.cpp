// ============================================================================
// Suraksha+ Watch OS — Battery Sensor Implementation
// ============================================================================
#include "battery.h"

// PMU battery helper declarations (from main.cpp)
extern int getPMUBatteryPercent();
extern uint16_t getPMUBatteryVoltage();
extern bool isPMUCharging();

bool BatterySensor::init() {
#if defined(PIN_BATTERY_ADC) && PIN_BATTERY_ADC >= 0
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    pinMode(PIN_BATTERY_ADC, INPUT);
#endif

    // Initial reading
    update();

    _initialized = true;
    Serial.printf("[BAT] ✅ Battery monitor initialized: %dmV (%d%%, Charging: %s)\n",
                  _data.voltage_mV, _data.percentage, _data.isCharging ? "YES" : "NO");
    return true;
}

void BatterySensor::update() {
    int pmuPct = getPMUBatteryPercent();
    uint16_t pmuMv = getPMUBatteryVoltage();
    bool charging = isPMUCharging();

    // Prefer PMU fuel gauge data (AXP2101)
    if (pmuMv > 0) {
        _data.voltage_mV = pmuMv;
    } else {
        _data.voltage_mV = _readVoltage();
    }

    if (pmuPct >= 0 && pmuPct <= 100) {
        _data.percentage = (uint8_t)pmuPct;
    } else if (_data.voltage_mV > 3000) {
        _data.percentage = _voltageToPercent(_data.voltage_mV);
    } else {
        _data.percentage = 100; // Default when powered via USB without battery
    }

    _data.isCharging = charging;
    _data.isLow = (_data.percentage <= BATTERY_LOW_PCT);
    _data.isCritical = (_data.percentage <= BATTERY_CRITICAL_PCT);
    _data.timestamp = millis();
}

uint16_t BatterySensor::_readVoltage() {
    // Read ADC multiple times for stability
    uint32_t sum = 0;
    for (int i = 0; i < 4; i++) {
        sum += analogReadMilliVolts(PIN_BATTERY_ADC);
    }
    uint16_t adcMv = sum / 4;

    // Apply voltage divider compensation
    uint16_t batteryMv = (uint16_t)(adcMv * BATTERY_VOLTAGE_DIVIDER);

    return batteryMv;
}

uint8_t BatterySensor::_voltageToPercent(uint16_t mv) {
    // LiPo discharge curve (3.7V nominal)
    // Non-linear mapping for accurate percentage
    if (mv >= 4200) return 100;
    if (mv >= 4100) return 90 + (mv - 4100) * 10 / 100;
    if (mv >= 4000) return 75 + (mv - 4000) * 15 / 100;
    if (mv >= 3900) return 55 + (mv - 3900) * 20 / 100;
    if (mv >= 3800) return 35 + (mv - 3800) * 20 / 100;
    if (mv >= 3700) return 20 + (mv - 3700) * 15 / 100;
    if (mv >= 3600) return 10 + (mv - 3600) * 10 / 100;
    if (mv >= 3500) return 5  + (mv - 3500) * 5  / 100;
    if (mv >= 3300) return (mv - 3300) * 5 / 200;
    return 0;
}
