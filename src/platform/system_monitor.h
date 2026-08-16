// ============================================================================
// Suraksha+ Watch OS — System Monitor
// Heap, task stack, and performance monitoring
// ============================================================================
#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <Arduino.h>

class SystemMonitor {
public:
    static SystemMonitor& getInstance();

    void init();
    void update();  // Call periodically (e.g., every 5s)

    // Memory info
    uint32_t getFreeHeap() const;
    uint32_t getMinFreeHeap() const;
    uint32_t getFreePsram() const;
    uint32_t getTotalPsram() const;

    // CPU info
    uint32_t getCpuFreqMHz() const;
    float getCpuTemperature() const;

    // Uptime
    uint32_t getUptimeSeconds() const;

    // Print system info to serial
    void printStatus() const;

private:
    SystemMonitor() = default;
    uint32_t _lastPrintMs = 0;
};

#endif // SYSTEM_MONITOR_H
