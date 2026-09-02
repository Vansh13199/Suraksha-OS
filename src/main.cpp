// ============================================================================
// Suraksha+ Watch OS — Main Entry Point
// Target: Seeed XIAO ESP32S3
// Display: SH1106 128x64 OLED (I2C)
// ============================================================================
//
// FreeRTOS Task Layout:
//   Core 0: Safety Engine (P6), Sensor Reading (P5), BLE (P3)
//   Core 1: UI (P4), Communication (P2), App Background (P1)
//
// Boot Sequence:
//   1. Hardware Init (I2C, GPIO)
//   2. Platform Init (Storage, Watchdog, Power)
//   3. Driver Init (Display, Vibration)
//   4. Sensor Init (HR, IMU, Battery, GPS)
//   5. Safety Engine Init
//   6. Communication Init (BLE)
//   7. UI Init (U8g2 + Screens)
//   8. Start FreeRTOS Tasks
//
// ============================================================================

#include <Arduino.h>
#include <Wire.h>

// Config
#include "config/hardware_config.h"
#include "config/ble_config.h"
#include "config/app_config.h"

// Platform
#include "platform/power_manager.h"
#include "platform/watchdog.h"
#include "platform/storage.h"
#include "platform/system_monitor.h"

// Drivers
#include "drivers/display_driver.h"
#include "drivers/vibration.h"
#include "drivers/buzzer.h"

// Sensors
#include "sensors/sensor_manager.h"

// Safety
#include "safety/safety_engine.h"

// Communication
#include "comms/ble_service.h"
#include "comms/ble_protocol.h"
#include "comms/sos_routing.h"
#include "comms/phone_bridge.h"

// UI
#include "ui/ui_manager.h"

// ============================================================================
// Task Handles
// ============================================================================
static TaskHandle_t taskSafetyHandle = NULL;
static TaskHandle_t taskSensorHandle = NULL;
static TaskHandle_t taskUIHandle = NULL;
static TaskHandle_t taskBLEHandle = NULL;
static TaskHandle_t taskCommsHandle = NULL;

// ============================================================================
// Safety Engine Task (Core 0 — Highest Priority)
// This task MUST keep running even if other tasks fail
// ============================================================================
void taskSafety(void* parameter) {
    Serial.println("[TASK] Safety engine started on Core 0");

    while (true) {
        SafetyEngine::getInstance().update();
        Watchdog::getInstance().feed("safety");
        vTaskDelay(pdMS_TO_TICKS(20)); // 50Hz
    }
}

// ============================================================================
// Sensor Reading Task (Core 0)
// ============================================================================
void taskSensor(void* parameter) {
    Serial.println("[TASK] Sensor reading started on Core 0");

    while (true) {
        SensorManager::getInstance().update();
        Watchdog::getInstance().feed("sensor");
        vTaskDelay(pdMS_TO_TICKS(20)); // 50Hz
    }
}

// ============================================================================
// U8g2 UI Task (Core 1)
// ============================================================================
void taskUI(void* parameter) {
    Serial.println("[TASK] U8g2 UI started on Core 1");

    while (true) {
        UIManager::getInstance().update();
        Vibration::getInstance().update();
        PowerManager::getInstance().update();
        Watchdog::getInstance().feed("ui");
        vTaskDelay(pdMS_TO_TICKS(UI_TICK_MS));
    }
}

// ============================================================================
// BLE Task (Core 0)
// ============================================================================
void taskBLE(void* parameter) {
    Serial.println("[TASK] BLE task started on Core 0");

    while (true) {
        BleService::getInstance().update();
        Watchdog::getInstance().feed("ble");
        vTaskDelay(pdMS_TO_TICKS(100)); // 10Hz
    }
}

// ============================================================================
// Communication Task (Core 1)
// Handles WiFi, HTTP, phone bridge, SOS routing
// ============================================================================
void taskComms(void* parameter) {
    Serial.println("[TASK] Communication task started on Core 1");

    while (true) {
        // Phone bridge periodic updates
        PhoneBridge::getInstance().update();

        // System monitor
        SystemMonitor::getInstance().update();

        // Watchdog supervisor
        Watchdog::getInstance().check();

        // If SOS is active, handle live updates via routing
        SafetyEngine& safety = SafetyEngine::getInstance();
        if (safety.isSOSActive()) {
            SOSManager& sos = safety.getSOSManager();
            SensorManager& sensors = SensorManager::getInstance();

            // Check if it's time for a live update
            static uint32_t lastSOSCommsUpdate = 0;
            if (millis() - lastSOSCommsUpdate >= SOS_LIVE_UPDATE_INTERVAL_MS) {
                lastSOSCommsUpdate = millis();

                char lat[16], lon[16];
                sensors.getGPS().getLatStr(lat, sizeof(lat));
                sensors.getGPS().getLonStr(lon, sizeof(lon));

                String phone = Storage::getInstance().getString(NVS_KEY_PHONE, "");

                SOSRouting::getInstance().sendLiveUpdate(
                    phone.c_str(),
                    sos.getSessionId(),
                    lat, lon,
                    sensors.getBPM()
                );
            }
        }

        Watchdog::getInstance().feed("comms");
        vTaskDelay(pdMS_TO_TICKS(500)); // 2Hz
    }
}

// ============================================================================
// BLE Message Handler (called from BLE RX interrupt context)
// ============================================================================
void onBleMessage(const char* data, size_t len) {
    BleProtocol::getInstance().handleMessage(data, len);
}

// ============================================================================
// Safety Event Handler (called when safety state changes)
// ============================================================================
void onSafetyEvent(SafetyState state, const char* message) {
    UIManager& ui = UIManager::getInstance();

    switch (state) {
        case SafetyState::SOS_ACTIVE:
            ui.showSOSOverlay(true);

            // Trigger SOS via routing
            {
                SensorManager& sensors = SensorManager::getInstance();
                char lat[16], lon[16];
                sensors.getGPS().getLatStr(lat, sizeof(lat));
                sensors.getGPS().getLonStr(lon, sizeof(lon));
                String phone = Storage::getInstance().getString(NVS_KEY_PHONE, "");

                SOSRouting::getInstance().sendSOS(
                    phone.c_str(),
                    SafetyEngine::getInstance().getSOSManager().getSessionId(),
                    lat, lon,
                    sensors.getBPM()
                );
            }
            break;

        case SafetyState::IDLE:
            ui.showSOSOverlay(false);
            break;

        case SafetyState::SOS_COUNTDOWN:
            // Show countdown on screen
            // TODO: Create countdown overlay
            break;

        case SafetyState::ACTIVITY_MODE:
            // Activity indicator handled by watchface screen
            break;
    }
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("+===========================================+");
    Serial.println("|                                           |");
    Serial.println("|    SURAKSHA+ WATCH OS v" WATCH_OS_VERSION "             |");
    Serial.println("|    Target: XIAO ESP32S3                   |");
    Serial.println("|    Display: SH1106 128x64 OLED            |");
    Serial.println("|                                           |");
    Serial.println("+===========================================+");
    Serial.println();

    // === 1. Platform Init ===
    Serial.println("-- Phase 1: Platform --");
    Storage::getInstance().init();
    Watchdog::getInstance().init();
    PowerManager::getInstance().init();
    SystemMonitor::getInstance().init();

    // === 2. Hardware Init ===
    Serial.println("\n-- Phase 2: Hardware --");

    // I2C for OLED display + sensors (shared bus)
    Wire.begin(PIN_SENSOR_SDA, PIN_SENSOR_SCL);
    Wire.setClock(I2C_SPEED);
    Serial.printf("[I2C] Bus initialized: SDA=%d, SCL=%d @ %dHz\n",
                  PIN_SENSOR_SDA, PIN_SENSOR_SCL, I2C_SPEED);

    // === 3. Driver Init ===
    Serial.println("\n-- Phase 3: Drivers --");
    Vibration::getInstance().init();
    Buzzer::getInstance().init();

    // Boot vibration
    Vibration::getInstance().play(HapticPattern::SUCCESS);

    // === 4. Sensor Init ===
    Serial.println("\n-- Phase 4: Sensors --");
    SensorManager::getInstance().init();

    // === 5. Safety Engine Init ===
    Serial.println("\n-- Phase 5: Safety Engine --");
    SafetyEngine::getInstance().init();
    SafetyEngine::getInstance().setEventCallback(onSafetyEvent);

    // === 6. Communication Init ===
    Serial.println("\n-- Phase 6: Communication --");
    BleService::getInstance().init();
    BleService::getInstance().onReceive(onBleMessage);
    BleProtocol::getInstance().init();
    PhoneBridge::getInstance().init();

    // === 7. UI Init ===
    Serial.println("\n-- Phase 7: UI --");
    UIManager::getInstance().init();

    // === 8. Start FreeRTOS Tasks ===
    Serial.println("\n-- Phase 8: FreeRTOS Tasks --");

    xTaskCreatePinnedToCore(taskSafety, "safety", TASK_STACK_SAFETY,
                            NULL, TASK_PRIORITY_SAFETY, &taskSafetyHandle, TASK_CORE_SAFETY);

    xTaskCreatePinnedToCore(taskSensor, "sensor", TASK_STACK_SENSOR,
                            NULL, TASK_PRIORITY_SENSOR, &taskSensorHandle, TASK_CORE_SENSOR);

    xTaskCreatePinnedToCore(taskUI, "ui", TASK_STACK_UI,
                            NULL, TASK_PRIORITY_UI, &taskUIHandle, TASK_CORE_UI);

    xTaskCreatePinnedToCore(taskBLE, "ble", TASK_STACK_BLE,
                            NULL, TASK_PRIORITY_BLE, &taskBLEHandle, TASK_CORE_BLE);

    xTaskCreatePinnedToCore(taskComms, "comms", TASK_STACK_COMMS,
                            NULL, TASK_PRIORITY_COMMS, &taskCommsHandle, TASK_CORE_COMMS);

    // Register tasks with watchdog
    Watchdog& wdt = Watchdog::getInstance();
    wdt.registerTask("safety", taskSafetyHandle, WDT_TASK_TIMEOUT_MS);
    wdt.registerTask("sensor", taskSensorHandle, WDT_TASK_TIMEOUT_MS);
    wdt.registerTask("ui", taskUIHandle, WDT_TASK_TIMEOUT_MS * 2);
    wdt.registerTask("ble", taskBLEHandle, WDT_TASK_TIMEOUT_MS * 3);
    wdt.registerTask("comms", taskCommsHandle, WDT_TASK_TIMEOUT_MS * 3);

    Serial.println();
    Serial.println("+===========================================+");
    Serial.println("|    SURAKSHA+ WATCH OS READY               |");
    Serial.printf("|    Free Heap: %6d bytes                 |\n", ESP.getFreeHeap());
    Serial.printf("|    Free PSRAM: %5d KB                    |\n", ESP.getFreePsram() / 1024);
    Serial.println("+===========================================+");
}

// ============================================================================
// LOOP (not used — all work is done in FreeRTOS tasks)
// ============================================================================
void loop() {
    // Arduino loop runs as lowest-priority task
    // We use it for serial debug commands only

    if (Serial.available()) {
        char cmd = Serial.read();
        switch (cmd) {
            case 's': // Manual SOS trigger
                Serial.println("[DEBUG] Manual SOS trigger");
                SafetyEngine::getInstance().triggerSOS("DEBUG_SERIAL");
                break;
            case 'x': // Manual SOS stop
                Serial.println("[DEBUG] Manual SOS stop");
                SafetyEngine::getInstance().stopSOS();
                break;
            case 'a': // Toggle activity mode
                if (SafetyEngine::getInstance().isActivityMode()) {
                    SafetyEngine::getInstance().stopActivityMode();
                } else {
                    SafetyEngine::getInstance().startActivityMode(300); // 5 min
                }
                break;
            case 'i': // System info
                SystemMonitor::getInstance().printStatus();
                break;
            case 'r': // Restart
                PowerManager::getInstance().restart();
                break;
        }
    }

    vTaskDelay(pdMS_TO_TICKS(1000)); // Low-priority idle
}
