// ============================================================================
// Suraksha+ Watch OS — BLE Service Implementation
// ============================================================================
#include "ble_service.h"
#include "esp_bt.h"
#include <cstdarg>

// --- BLE Server Callbacks ---
class WatchServerCallbacks : public BLEServerCallbacks {
public:
    WatchServerCallbacks(BleService* svc) : _svc(svc) {}

    void onConnect(BLEServer* pServer) override {
        _svc->_connected = true;
        Serial.println("[BLE] 📱 Phone connected");
        if (_svc->_connCallback) _svc->_connCallback(true);
    }

    void onDisconnect(BLEServer* pServer) override {
        _svc->_connected = false;
        Serial.println("[BLE] 📱 Phone disconnected");
        if (_svc->_connCallback) _svc->_connCallback(false);
    }

private:
    BleService* _svc;
};

// --- BLE RX Callbacks ---
class WatchRxCallbacks : public BLECharacteristicCallbacks {
public:
    WatchRxCallbacks(BleService* svc) : _svc(svc) {}

    void onWrite(BLECharacteristic* pChar) override {
        String rxValue = pChar->getValue().c_str();
        if (rxValue.length() > 0) {
            Serial.printf("[BLE] RX: %s\n", rxValue.c_str());
            if (_svc->_rxCallback) {
                _svc->_rxCallback(rxValue.c_str(), rxValue.length());
            }
        }
    }

private:
    BleService* _svc;
};

// --- Singleton ---
BleService& BleService::getInstance() {
    static BleService instance;
    return instance;
}

void BleService::init() {
    Serial.println("[BLE] Initializing BLE service...");

    // Release Classic BT memory (same as existing firmware)
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    Serial.printf("[BLE] Classic BT released. Free heap: %d\n", ESP.getFreeHeap());

    // Initialize BLE
    BLEDevice::init(BLE_DEVICE_NAME);
    BLEDevice::setMTU(512);

    // Create server
    _pServer = BLEDevice::createServer();
    _pServer->setCallbacks(new WatchServerCallbacks(this));

    // Create service (same UUID as existing firmware)
    BLEService* pService = _pServer->createService(BLE_SERVICE_UUID);

    // TX Characteristic (Watch → Phone): Notify
    _pTxChar = pService->createCharacteristic(
        BLE_CHAR_TX_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    _pTxChar->addDescriptor(new BLE2902());

    // RX Characteristic (Phone → Watch): Write
    _pRxChar = pService->createCharacteristic(
        BLE_CHAR_RX_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    _pRxChar->setCallbacks(new WatchRxCallbacks(this));

    // Start service
    pService->start();

    // Start advertising
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
    pAdvertising->setScanResponse(BLE_SCAN_RESPONSE);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12); // Help with iPhone connection issues
    BLEDevice::startAdvertising();

    Serial.printf("[BLE] ✅ Advertising as '%s'\n", BLE_DEVICE_NAME);
    Serial.printf("[BLE] Service: %s\n", BLE_SERVICE_UUID);
}

void BleService::update() {
    // Handle reconnection advertising
    if (!_connected && _wasConnected) {
        delay(BLE_RECONNECT_DELAY_MS);
        _pServer->startAdvertising();
        Serial.println("[BLE] Re-advertising...");
        _wasConnected = false;
    }
    if (_connected && !_wasConnected) {
        _wasConnected = true;
    }
}

void BleService::send(const char* msg) {
    if (!_connected || !_pTxChar) return;
    _pTxChar->setValue(msg);
    _pTxChar->notify();
    Serial.printf("[BLE] TX: %s\n", msg);
}

void BleService::sendFormatted(const char* fmt, ...) {
    if (!_connected) return;

    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    send(buf);
}
