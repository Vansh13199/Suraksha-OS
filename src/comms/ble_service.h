// ============================================================================
// Suraksha+ Watch OS — BLE Service
// Nordic UART Service compatible with existing Flutter app
// ============================================================================
#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "../config/ble_config.h"

// Callback types
typedef void (*BleRxCallback)(const char* data, size_t len);
typedef void (*BleConnectionCallback)(bool connected);

class BleService {
public:
    static BleService& getInstance();

    void init();
    void update();

    // Send data to phone
    void send(const char* msg);
    void sendFormatted(const char* fmt, ...);

    // Connection status
    bool isConnected() const { return _connected; }

    // Callbacks
    void onReceive(BleRxCallback cb) { _rxCallback = cb; }
    void onConnection(BleConnectionCallback cb) { _connCallback = cb; }

private:
    BleService() = default;

    BLEServer* _pServer = nullptr;
    BLECharacteristic* _pTxChar = nullptr;
    BLECharacteristic* _pRxChar = nullptr;
    bool _connected = false;
    bool _wasConnected = false;

    BleRxCallback _rxCallback = nullptr;
    BleConnectionCallback _connCallback = nullptr;

    // BLE Callbacks (friend classes for access)
    friend class WatchServerCallbacks;
    friend class WatchRxCallbacks;
};

#endif // BLE_SERVICE_H
