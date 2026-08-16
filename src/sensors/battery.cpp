// ============================================================================
// Suraksha+ Watch OS — Battery Sensor Implementation
// ============================================================================
#include "battery.h"

bool BatterySensor::init() {
    // Configure ADC
    analogReadResolution(12);               // 12-bit ADC (0-4095)
    analogSetAttenuation(ADC_11db);         // Full scale ~3.3V
    pinMode(PIN_BATTERY_ADC, INPUT);

    // Initial reading
    update();

    _initialized = true;
    Serial.printf("[BAT] ✅ Battery monitor initialized: %dmV (%d%%)\n",
                  _data.voltage_mV, _data.percentage);
    return true;
}

void BatterySensor::update() {
    uint16_t voltage = _readVoltage();

    // Add to averaging buffer
    _voltageBuf[_bufIdx] = voltage;
    _bufIdx = (_bufIdx + 1) % AVG_SAMPLES;
    if (_bufCount < AVG_SAMPLES) _bufCount++;

    // Calculate average voltage
    uint32_t sum = 0;
    for (int i = 0; i < _bufCount; i++) {
        sum += _voltageBuf[i];
    }
    uint16_t avgVoltage = sum / _bufCount;

    _data.voltage_mV = avgVoltage;
    _data.percentage = _voltageToPercent(avgVoltage);
    _data.isLow = (_data.percentage <= BATTERY_LOW_PCT);
    _data.isCritical = (_data.percentage <= BATTERY_CRITICAL_PCT);
    _data.timestamp = millis();

    // Charging detection: if voltage is above full charge
    // (simple heuristic — better with dedicated charge IC status pin)
    _data.isCharging = (avgVoltage > BATTERY_FULL_MV);
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
