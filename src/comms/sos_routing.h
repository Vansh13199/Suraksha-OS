// ============================================================================
// Suraksha+ Watch OS — SOS Routing Engine
// Determines optimal communication path for SOS
// ============================================================================
#ifndef SOS_ROUTING_H
#define SOS_ROUTING_H

#include <Arduino.h>

enum class SOSRoute {
    BLE_TO_PHONE,       // Send via BLE → Phone handles everything
    WIFI_DIRECT,        // Direct WiFi → AWS API call
    STORE_AND_FORWARD,  // Queue for later transmission
    NONE                // No route available
};

class SOSRouting {
public:
    static SOSRouting& getInstance();

    // Determine best route
    SOSRoute getBestRoute() const;

    // Execute SOS via best available route
    bool sendSOS(const char* phoneNumber, const char* sessionId,
                 const char* lat, const char* lon, float heartRate);

    bool stopSOS(const char* sessionId);

    // Live location update
    bool sendLiveUpdate(const char* phoneNumber, const char* sessionId,
                        const char* lat, const char* lon, float heartRate);

private:
    SOSRouting() = default;

    bool _sendViaBLE(const char* sessionId, const char* lat, const char* lon);
    bool _sendViaWiFi(const char* phoneNumber, const char* sessionId,
                      const char* lat, const char* lon, float heartRate);
    bool _sendViaHTTP(const char* url, const char* payload);
};

#endif // SOS_ROUTING_H
