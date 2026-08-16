// ============================================================================
// Suraksha+ Watch OS — BLE Protocol Handler
// Parses incoming BLE messages and dispatches them
// ============================================================================
#ifndef BLE_PROTOCOL_H
#define BLE_PROTOCOL_H

#include <Arduino.h>
#include "../config/ble_config.h"

class BleProtocol {
public:
    static BleProtocol& getInstance();

    void init();

    // Process incoming BLE message (called from BLE RX callback)
    void handleMessage(const char* data, size_t len);

private:
    BleProtocol() = default;

    void _handleSetPhone(const char* data);
    void _handleActivity(const char* data);
    void _handleTimeSync(const char* data);
    void _handleNotification(const char* data);
    void _handleGPSData(const char* data);
    void _handleSettings(const char* data);
    void _handleAIResponse(const char* data);
    void _handleDND(const char* data);
    void _handleSOSCommand(const char* data);
};

#endif // BLE_PROTOCOL_H
