// ============================================================================
// Suraksha+ Watch OS — GPS Sensor
// Dual-source: hardware UART module or phone relay via BLE
// ============================================================================
#ifndef GPS_H
#define GPS_H

#include <Arduino.h>
#include "../config/hardware_config.h"

enum class GPSSource {
    NONE,
    HARDWARE,       // Direct UART GPS module
    PHONE_BLE       // Relayed from phone via BLE
};

struct GPSData {
    double latitude;
    double longitude;
    double altitude;
    float speed;            // km/h
    uint8_t satellites;
    bool hasFix;
    GPSSource source;
    uint32_t timestamp;     // Last valid fix time
};

class GPSSensor {
public:
    bool init();
    void update();          // Call to process UART data
    GPSData getData() const { return _data; }

    // Set GPS data from BLE relay (phone → watch)
    void setPhoneGPS(double lat, double lon, double alt = 0, float speed = 0);

    // Get formatted coordinate strings (for SOS API)
    void getLatStr(char* buf, size_t len) const;
    void getLonStr(char* buf, size_t len) const;
    
    // Reset to hardcoded default coordinates
    void resetToDefault();

    bool hasFix() const { return _data.hasFix; }
    bool isStale() const;   // True if data is older than 30s

private:
    void _parseNMEA(const char* sentence);
    void _parseGGA(const char* sentence);
    void _parseRMC(const char* sentence);
    double _nmeaToDecimal(const char* nmeaCoord, char hemisphere);

    GPSData _data = {};
    bool _hardwareAvailable = false;

    // NMEA parsing buffer
    char _nmeaBuf[128] = {};
    uint8_t _nmeaIdx = 0;

    static const uint32_t STALE_TIMEOUT_MS = 30000; // 30s
};

#endif // GPS_H
