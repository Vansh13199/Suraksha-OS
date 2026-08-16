// ============================================================================
// Suraksha+ Watch OS — Watchdog Implementation
// ============================================================================
#include "watchdog.h"
#include "../config/app_config.h"
#include <Preferences.h>

Watchdog& Watchdog::getInstance() {
    static Watchdog instance;
    return instance;
}

void Watchdog::init() {
    // Initialize hardware watchdog
    esp_task_wdt_init(WDT_TIMEOUT_S, true); // true = panic on timeout

    // Check if this was a clean boot
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    _cleanBoot = prefs.getBool("clean_sleep", false);
    prefs.end();

    if (!_cleanBoot) {
        // Increment crash counter
        prefs.begin(NVS_NAMESPACE, false);
        uint32_t crashes = prefs.getUInt(NVS_KEY_CRASH_COUNT, 0) + 1;
        prefs.putUInt(NVS_KEY_CRASH_COUNT, crashes);
        prefs.end();
        Serial.printf("[WDT] ⚠️ Unclean boot detected! Crash count: %d\n", crashes);
    } else {
        // Clear clean_sleep flag (will be set again on clean sleep entry)
        prefs.begin(NVS_NAMESPACE, false);
        prefs.putBool("clean_sleep", false);
        prefs.end();
    }

    Serial.println("[WDT] Watchdog initialized");
}

void Watchdog::registerTask(const char* name, TaskHandle_t handle, uint32_t timeoutMs) {
    if (_taskCount >= MAX_MONITORED_TASKS) {
        Serial.println("[WDT] ERROR: Max monitored tasks reached!");
        return;
    }

    _tasks[_taskCount] = {
        .name = name,
        .handle = handle,
        .timeoutMs = timeoutMs,
        .lastFeedMs = millis(),
        .registered = true
    };
    _taskCount++;

    // Add to hardware WDT
    esp_task_wdt_add(handle);

    Serial.printf("[WDT] Registered task: %s (timeout: %dms)\n", name, timeoutMs);
}

void Watchdog::feed(const char* taskName) {
    for (uint8_t i = 0; i < _taskCount; i++) {
        if (_tasks[i].registered && strcmp(_tasks[i].name, taskName) == 0) {
            _tasks[i].lastFeedMs = millis();
            // Also feed the hardware WDT
            esp_task_wdt_reset();
            return;
        }
    }
}

void Watchdog::check() {
    uint32_t now = millis();
    for (uint8_t i = 0; i < _taskCount; i++) {
        if (!_tasks[i].registered) continue;

        uint32_t elapsed = now - _tasks[i].lastFeedMs;
        if (elapsed > _tasks[i].timeoutMs) {
            Serial.printf("[WDT] ⚠️ Task '%s' unresponsive for %dms!\n",
                         _tasks[i].name, elapsed);

            // If it's not the safety engine, try to recover
            if (strcmp(_tasks[i].name, "safety") != 0) {
                // Could delete and recreate the task here
                // For now, just log the warning
                Serial.printf("[WDT] Task '%s' may need restart\n", _tasks[i].name);
            }

            // If multiple tasks are hung, it's likely a system issue
            int hungCount = 0;
            for (uint8_t j = 0; j < _taskCount; j++) {
                if (_tasks[j].registered &&
                    (now - _tasks[j].lastFeedMs > _tasks[j].timeoutMs)) {
                    hungCount++;
                }
            }
            if (hungCount >= 3) {
                Serial.println("[WDT] 🚨 Multiple tasks hung — forcing restart!");
                ESP.restart();
            }
        }
    }
}

uint32_t Watchdog::getCrashCount() const {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    uint32_t count = prefs.getUInt(NVS_KEY_CRASH_COUNT, 0);
    prefs.end();
    return count;
}

void Watchdog::clearCrashCount() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUInt(NVS_KEY_CRASH_COUNT, 0);
    prefs.end();
}
