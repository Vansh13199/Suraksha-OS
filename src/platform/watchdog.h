// ============================================================================
// Suraksha+ Watch OS — Watchdog & Recovery
// Monitors task health and recovers from crashes
// ============================================================================
#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <Arduino.h>
#include <esp_task_wdt.h>

#define MAX_MONITORED_TASKS 8

class Watchdog {
public:
    static Watchdog& getInstance();

    void init();

    // Register a task for monitoring
    void registerTask(const char* name, TaskHandle_t handle, uint32_t timeoutMs);

    // Call from each task to report it's alive
    void feed(const char* taskName);

    // Check for hung tasks (call from a supervisor task)
    void check();

    // Crash recovery
    uint32_t getCrashCount() const;
    void clearCrashCount();
    bool wasCleanBoot() const { return _cleanBoot; }

private:
    Watchdog() = default;

    struct MonitoredTask {
        const char* name;
        TaskHandle_t handle;
        uint32_t timeoutMs;
        uint32_t lastFeedMs;
        bool registered;
    };

    MonitoredTask _tasks[MAX_MONITORED_TASKS] = {};
    uint8_t _taskCount = 0;
    bool _cleanBoot = true;
};

#endif // WATCHDOG_H
