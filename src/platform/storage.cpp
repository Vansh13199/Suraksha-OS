// ============================================================================
// Suraksha+ Watch OS — Storage Manager Implementation
// ============================================================================
#include "storage.h"

Storage& Storage::getInstance() {
    static Storage instance;
    return instance;
}

void Storage::init() {
    Serial.println("[STOR] Storage initialized");
}

void Storage::_ensureOpen() {
    if (!_opened) {
        _prefs.begin(NVS_NAMESPACE, false);
        _opened = true;
    }
}

void Storage::_close() {
    if (_opened) {
        _prefs.end();
        _opened = false;
    }
}

String Storage::getString(const char* key, const String& defaultVal) {
    _ensureOpen();
    String val = _prefs.getString(key, defaultVal);
    _close();
    return val;
}

void Storage::putString(const char* key, const String& value) {
    _ensureOpen();
    _prefs.putString(key, value);
    _close();
}

int32_t Storage::getInt(const char* key, int32_t defaultVal) {
    _ensureOpen();
    int32_t val = _prefs.getInt(key, defaultVal);
    _close();
    return val;
}

void Storage::putInt(const char* key, int32_t value) {
    _ensureOpen();
    _prefs.putInt(key, value);
    _close();
}

uint32_t Storage::getUInt(const char* key, uint32_t defaultVal) {
    _ensureOpen();
    uint32_t val = _prefs.getUInt(key, defaultVal);
    _close();
    return val;
}

void Storage::putUInt(const char* key, uint32_t value) {
    _ensureOpen();
    _prefs.putUInt(key, value);
    _close();
}

uint8_t Storage::getByte(const char* key, uint8_t defaultVal) {
    _ensureOpen();
    uint8_t val = _prefs.getUChar(key, defaultVal);
    _close();
    return val;
}

void Storage::putByte(const char* key, uint8_t value) {
    _ensureOpen();
    _prefs.putUChar(key, value);
    _close();
}

bool Storage::getBool(const char* key, bool defaultVal) {
    _ensureOpen();
    bool val = _prefs.getBool(key, defaultVal);
    _close();
    return val;
}

void Storage::putBool(const char* key, bool value) {
    _ensureOpen();
    _prefs.putBool(key, value);
    _close();
}

float Storage::getFloat(const char* key, float defaultVal) {
    _ensureOpen();
    float val = _prefs.getFloat(key, defaultVal);
    _close();
    return val;
}

void Storage::putFloat(const char* key, float value) {
    _ensureOpen();
    _prefs.putFloat(key, value);
    _close();
}

size_t Storage::getBytes(const char* key, void* buf, size_t maxLen) {
    _ensureOpen();
    size_t read = _prefs.getBytes(key, buf, maxLen);
    _close();
    return read;
}

void Storage::putBytes(const char* key, const void* buf, size_t len) {
    _ensureOpen();
    _prefs.putBytes(key, buf, len);
    _close();
}

void Storage::remove(const char* key) {
    _ensureOpen();
    _prefs.remove(key);
    _close();
}

void Storage::factoryReset() {
    _ensureOpen();
    _prefs.clear();
    _close();
    Serial.println("[STOR] ⚠️ Factory reset — all settings cleared");
}
