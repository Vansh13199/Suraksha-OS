// ============================================================================
// Suraksha+ Watch OS — Phone Bridge
// Bidirectional data sync between watch and phone
// ============================================================================
#ifndef PHONE_BRIDGE_H
#define PHONE_BRIDGE_H

#include <Arduino.h>

class PhoneBridge {
public:
    static PhoneBridge& getInstance();

    void init();
    void update();  // Call periodically

    // Push data to phone
    void sendHealthData();
    void sendBatteryStatus();
    void sendSensorData();
    void requestGPS();
    void findPhone();
    void sendAIQuery(const char* query);

    // Sync status
    uint32_t getLastSyncMs() const { return _lastSyncMs; }

private:
    PhoneBridge() = default;
    uint32_t _lastSyncMs = 0;
    uint32_t _lastHealthPushMs = 0;
    uint32_t _lastBatteryPushMs = 0;

    static const uint32_t HEALTH_PUSH_INTERVAL_MS = 30000;  // 30s
    static const uint32_t BATTERY_PUSH_INTERVAL_MS = 60000; // 60s
};

#endif // PHONE_BRIDGE_H
