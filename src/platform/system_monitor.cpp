// ============================================================================
// Suraksha+ Watch OS — System Monitor Implementation
// ============================================================================
#include "system_monitor.h"
#include <esp_system.h>
#include <esp_heap_caps.h>

SystemMonitor& SystemMonitor::getInstance() {
    static SystemMonitor instance;
    return instance;
}

void SystemMonitor::init() {
    Serial.println("[MON] System monitor initialized");
    printStatus();
}

void SystemMonitor::update() {
    uint32_t now = millis();
    if (now - _lastPrintMs >= 30000) { // Every 30 seconds
        _lastPrintMs = now;
        printStatus();
    }
}

uint32_t SystemMonitor::getFreeHeap() const {
    return ESP.getFreeHeap();
}

uint32_t SystemMonitor::getMinFreeHeap() const {
    return ESP.getMinFreeHeap();
}

uint32_t SystemMonitor::getFreePsram() const {
    return ESP.getFreePsram();
}

uint32_t SystemMonitor::getTotalPsram() const {
    return ESP.getPsramSize();
}

uint32_t SystemMonitor::getCpuFreqMHz() const {
    return ESP.getCpuFreqMHz();
}

float SystemMonitor::getCpuTemperature() const {
    // ESP32-S3 has internal temperature sensor
    // Note: accuracy is ±2°C
    return temperatureRead();
}

uint32_t SystemMonitor::getUptimeSeconds() const {
    return millis() / 1000;
}

void SystemMonitor::printStatus() const {
    Serial.println("╔═══════════════════════════════════╗");
    Serial.println("║     SYSTEM STATUS                 ║");
    Serial.println("╠═══════════════════════════════════╣");
    Serial.printf("║ Heap Free: %6d KB / Min: %4d KB║\n",
                  getFreeHeap() / 1024, getMinFreeHeap() / 1024);
    Serial.printf("║ PSRAM Free: %5d KB / %5d KB   ║\n",
                  getFreePsram() / 1024, getTotalPsram() / 1024);
    Serial.printf("║ CPU: %3d MHz  Temp: %.1f°C        ║\n",
                  getCpuFreqMHz(), getCpuTemperature());
    Serial.printf("║ Uptime: %d seconds               ║\n",
                  getUptimeSeconds());
    Serial.println("╚═══════════════════════════════════╝");
}
