// ============================================================================
// Suraksha+ Watch OS — Storage Manager
// NVS-based persistent configuration storage
// ============================================================================
#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <Preferences.h>
#include "../config/app_config.h"

class Storage {
public:
    static Storage& getInstance();

    void init();

    // String values
    String getString(const char* key, const String& defaultVal = "");
    void putString(const char* key, const String& value);

    // Integer values
    int32_t getInt(const char* key, int32_t defaultVal = 0);
    void putInt(const char* key, int32_t value);

    // Unsigned integer values
    uint32_t getUInt(const char* key, uint32_t defaultVal = 0);
    void putUInt(const char* key, uint32_t value);

    // Byte values
    uint8_t getByte(const char* key, uint8_t defaultVal = 0);
    void putByte(const char* key, uint8_t value);

    // Boolean values
    bool getBool(const char* key, bool defaultVal = false);
    void putBool(const char* key, bool value);

    // Float values
    float getFloat(const char* key, float defaultVal = 0.0f);
    void putFloat(const char* key, float value);

    // Raw bytes
    size_t getBytes(const char* key, void* buf, size_t maxLen);
    void putBytes(const char* key, const void* buf, size_t len);

    // Remove a key
    void remove(const char* key);

    // Factory reset
    void factoryReset();

private:
    Storage() = default;
    Preferences _prefs;
    bool _opened = false;

    void _ensureOpen();
    void _close();
};

#endif // STORAGE_H
