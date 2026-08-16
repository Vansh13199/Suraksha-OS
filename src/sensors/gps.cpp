// ============================================================================
// Suraksha+ Watch OS — GPS Sensor Implementation
// ============================================================================
#include "gps.h"

bool GPSSensor::init() {
#if GPS_HARDWARE_ENABLED
    GPS_SERIAL.begin(GPS_BAUD_RATE, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
    _hardwareAvailable = true;
    Serial.println("[GPS] ✅ Hardware GPS initialized (UART)");
#else
    _hardwareAvailable = false;
    Serial.println("[GPS] No hardware GPS — using phone relay via BLE");
#endif

    // Initialize with zero coordinates
    _data.latitude = 0.0;
    _data.longitude = 0.0;
    _data.hasFix = false;
    _data.source = GPSSource::NONE;

    return true;
}

void GPSSensor::update() {
#if GPS_HARDWARE_ENABLED
    if (!_hardwareAvailable) return;

    while (GPS_SERIAL.available()) {
        char c = GPS_SERIAL.read();

        if (c == '$') {
            _nmeaIdx = 0;
        }

        if (_nmeaIdx < sizeof(_nmeaBuf) - 1) {
            _nmeaBuf[_nmeaIdx++] = c;
        }

        if (c == '\n' || c == '\r') {
            _nmeaBuf[_nmeaIdx] = '\0';
            if (_nmeaIdx > 6) {
                _parseNMEA(_nmeaBuf);
            }
            _nmeaIdx = 0;
        }
    }
#endif
}

void GPSSensor::setPhoneGPS(double lat, double lon, double alt, float speed) {
    _data.latitude = lat;
    _data.longitude = lon;
    _data.altitude = alt;
    _data.speed = speed;
    _data.hasFix = (lat != 0.0 || lon != 0.0);
    _data.source = GPSSource::PHONE_BLE;
    _data.timestamp = millis();
}

void GPSSensor::getLatStr(char* buf, size_t len) const {
    snprintf(buf, len, "%.6f", _data.latitude);
}

void GPSSensor::getLonStr(char* buf, size_t len) const {
    snprintf(buf, len, "%.6f", _data.longitude);
}

bool GPSSensor::isStale() const {
    if (!_data.hasFix) return true;
    return (millis() - _data.timestamp) > STALE_TIMEOUT_MS;
}

void GPSSensor::_parseNMEA(const char* sentence) {
    if (strncmp(sentence, "$GPGGA", 6) == 0 || strncmp(sentence, "$GNGGA", 6) == 0) {
        _parseGGA(sentence);
    } else if (strncmp(sentence, "$GPRMC", 6) == 0 || strncmp(sentence, "$GNRMC", 6) == 0) {
        _parseRMC(sentence);
    }
}

void GPSSensor::_parseGGA(const char* sentence) {
    // $GPGGA,time,lat,N/S,lon,E/W,quality,sats,hdop,alt,M,...
    char fields[15][20];
    memset(fields, 0, sizeof(fields));

    int fieldIdx = 0;
    int charIdx = 0;

    for (int i = 0; sentence[i] != '\0' && fieldIdx < 15; i++) {
        if (sentence[i] == ',' || sentence[i] == '*') {
            fields[fieldIdx][charIdx] = '\0';
            fieldIdx++;
            charIdx = 0;
        } else {
            if (charIdx < 19) {
                fields[fieldIdx][charIdx++] = sentence[i];
            }
        }
    }

    // Field 6: Fix quality (0=none, 1=GPS, 2=DGPS)
    int quality = atoi(fields[6]);
    if (quality == 0) {
        _data.hasFix = false;
        return;
    }

    // Field 2,3: Latitude
    if (strlen(fields[2]) > 0 && strlen(fields[3]) > 0) {
        _data.latitude = _nmeaToDecimal(fields[2], fields[3][0]);
    }

    // Field 4,5: Longitude
    if (strlen(fields[4]) > 0 && strlen(fields[5]) > 0) {
        _data.longitude = _nmeaToDecimal(fields[4], fields[5][0]);
    }

    // Field 7: Satellites
    _data.satellites = atoi(fields[7]);

    // Field 9: Altitude
    if (strlen(fields[9]) > 0) {
        _data.altitude = atof(fields[9]);
    }

    _data.hasFix = true;
    _data.source = GPSSource::HARDWARE;
    _data.timestamp = millis();
}

void GPSSensor::_parseRMC(const char* sentence) {
    // $GPRMC,time,status,lat,N/S,lon,E/W,speed,course,date,...
    char fields[15][20];
    memset(fields, 0, sizeof(fields));

    int fieldIdx = 0;
    int charIdx = 0;

    for (int i = 0; sentence[i] != '\0' && fieldIdx < 15; i++) {
        if (sentence[i] == ',' || sentence[i] == '*') {
            fields[fieldIdx][charIdx] = '\0';
            fieldIdx++;
            charIdx = 0;
        } else {
            if (charIdx < 19) {
                fields[fieldIdx][charIdx++] = sentence[i];
            }
        }
    }

    // Field 2: Status (A=active, V=void)
    if (fields[2][0] != 'A') return;

    // Field 7: Speed (knots → km/h)
    if (strlen(fields[7]) > 0) {
        _data.speed = atof(fields[7]) * 1.852f;
    }
}

double GPSSensor::_nmeaToDecimal(const char* nmeaCoord, char hemisphere) {
    // NMEA format: DDMM.MMMM or DDDMM.MMMM
    double raw = atof(nmeaCoord);
    int degrees = (int)(raw / 100);
    double minutes = raw - (degrees * 100);
    double decimal = degrees + (minutes / 60.0);

    if (hemisphere == 'S' || hemisphere == 'W') {
        decimal = -decimal;
    }

    return decimal;
}
