// ============================================================================
// Suraksha+ Watch OS — Main Entry Point
// Target: watch
// ============================================================================
// Architecture:
//   - Heart rate data arrives via ESP-NOW from Seeed XIAO ESP32S3
//   - Display: ST7789 240x240 TFT via TFT_eSPI
//   - PMU: AXP2101 (battery, power rails)
//   - IMU: BMA423/BMA456 or MPU6050
//   - Touch: FT6336U capacitive touch
//   - Comms: BLE to phone, WiFi for NTP/HTTP, ESP-NOW for HR sensor
// ============================================================================

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <sys/time.h>

// --- Config ---
#include "config/hardware_config.h"
#include "config/app_config.h"
#include "config/ble_config.h"

// --- Platform ---
#include "platform/storage.h"
#include "platform/watchdog.h"
#include "platform/power_manager.h"
#include "platform/system_monitor.h"

// --- Drivers ---
#include "drivers/display_driver.h"
#include "drivers/vibration.h"

// --- Sensors ---
#include "sensors/sensor_manager.h"

// --- Safety ---
#include "safety/safety_engine.h"

// --- Comms ---
#include "comms/ble_service.h"
#include "comms/ble_protocol.h"
#include "comms/phone_bridge.h"
#include "comms/wifi_manager.h"
#include "comms/sos_routing.h"
#include "comms/espnow_receiver.h"

// --- UI ---
#include "ui/ui_manager.h"

#include <XPowersLib.h>

static XPowersPMU s_pmu;

bool initPMU() {
    Serial.println("[PMU] Initializing AXP2101 PMU...");
    if (!s_pmu.init(Wire, PIN_SENSOR_SDA, PIN_SENSOR_SCL, AXP2101_I2C_ADDR)) {
        Serial.println("[PMU] ⚠️ AXP2101 not responding at 0x34");
        return false;
    }
    Serial.println("[PMU] ✅ AXP2101 initialized!");

    // ALDO1-4: Peripheral power rails (Display backlight, display logic, touch, etc.)
    s_pmu.setALDO1Voltage(3300);
    s_pmu.enableALDO1();
    s_pmu.setALDO2Voltage(3300);
    s_pmu.enableALDO2();
    s_pmu.setALDO3Voltage(3300);
    s_pmu.enableALDO3();
    s_pmu.setALDO4Voltage(3300);
    s_pmu.enableALDO4();

    // BLDO1-2: Secondary peripheral power rails (GNSS, Haptic DRV2605)
    s_pmu.setBLDO1Voltage(3300);
    s_pmu.enableBLDO1();
    s_pmu.setBLDO2Voltage(3300);
    s_pmu.enableBLDO2();

    // Enable Battery & Power measurements on AXP2101
    s_pmu.enableBattDetection();
    s_pmu.enableBattVoltageMeasure();
    s_pmu.enableVbusVoltageMeasure();
    s_pmu.enableSystemVoltageMeasure();

    Serial.printf("[PMU] Battery: %d%% (%dmV) | VBUS: %s | Charging: %s\n",
                  s_pmu.getBatteryPercent(), s_pmu.getBattVoltage(),
                  s_pmu.isVbusIn() ? "YES" : "NO",
                  s_pmu.isCharging() ? "YES" : "NO");

    return true;
}

// Global PMU battery helpers for BatterySensor
int getPMUBatteryPercent() {
    return s_pmu.getBatteryPercent();
}

uint16_t getPMUBatteryVoltage() {
    return s_pmu.getBattVoltage();
}

bool isPMUCharging() {
    return s_pmu.isCharging() || s_pmu.isVbusIn();
}

// Global I2C mutex (shared between sensors, display, PMU)
SemaphoreHandle_t g_i2cMutex = nullptr;

// Safety Event Callback
void onSafetyEvent(SafetyState state, const char* message) {
    UIManager& ui = UIManager::getInstance();

    switch (state) {
        case SafetyState::SOS_ACTIVE: {
            Serial.printf("[MAIN] 🚨 SOS Activated: %s\n", message ? message : "UNKNOWN");
            ui.showSOSOverlay(true);
            Vibration::getInstance().play(HapticPattern::SOS_CONFIRM);

            // Dispatch SOS alert via network (BLE + WiFi/AWS)
            SensorManager& sensors = SensorManager::getInstance();
            char lat[16], lon[16];
            snprintf(lat, sizeof(lat), "%.6f", sensors.getLatitude());
            snprintf(lon, sizeof(lon), "%.6f", sensors.getLongitude());

            String phone = Storage::getInstance().getString(NVS_KEY_PHONE, DEFAULT_EMERGENCY_PHONE);
            if (phone.length() == 0) phone = DEFAULT_EMERGENCY_PHONE;

            const char* sessionId = SafetyEngine::getInstance().getSOSManager().getSessionId();
            Serial.printf("[MAIN] 📡 Dispatching SOS: phone=%s session=%s\n", phone.c_str(), sessionId);

            SOSRouting::getInstance().sendSOS(
                phone.c_str(),
                sessionId,
                lat, lon,
                sensors.getBPM()
            );
            break;
        }

        case SafetyState::IDLE: {
            Serial.println("[MAIN] Safety State: IDLE");
            ui.showSOSOverlay(false);

            if (message && strstr(message, "SOS stopped") != nullptr) {
                const char* sessionId = SafetyEngine::getInstance().getSOSManager().getSessionId();
                Serial.printf("[MAIN] 🛑 Stopping SOS on network: session=%s\n", sessionId);
                SOSRouting::getInstance().stopSOS(sessionId);
            }
            break;
        }

        case SafetyState::SOS_COUNTDOWN:
            Serial.println("[MAIN] ⚠️ Fall countdown active!");
            Vibration::getInstance().play(HapticPattern::WARNING);
            break;

        case SafetyState::ACTIVITY_MODE:
            Serial.println("[MAIN] Activity Mode Active");
            ui.showScreen(Screen::WATCHFACE);
            break;
    }
}

// Task handles for watchdog monitoring
static TaskHandle_t s_taskSafety = nullptr;
static TaskHandle_t s_taskSensor = nullptr;
static TaskHandle_t s_taskUI = nullptr;

// ============================================================================
// FreeRTOS Task Functions
// ============================================================================

void taskSafety(void* parameter) {
    Serial.println("[TASK] Safety engine started (Core 0)");
    for (;;) {
        SafetyEngine::getInstance().update();
        Watchdog::getInstance().feed("safety");
        vTaskDelay(pdMS_TO_TICKS(20));  // 50Hz
    }
}

void taskSensor(void* parameter) {
    Serial.println("[TASK] Sensor manager started (Core 0)");
    for (;;) {
        SensorManager::getInstance().update();
        Watchdog::getInstance().feed("sensor");
        vTaskDelay(pdMS_TO_TICKS(10));  // 100Hz for responsive HR updates
    }
}

void taskUI(void* parameter) {
    Serial.println("[TASK] UI manager started (Core 1)");
    for (;;) {
        UIManager::getInstance().update();
        Watchdog::getInstance().feed("ui");
        vTaskDelay(pdMS_TO_TICKS(UI_TICK_MS));
    }
}

void taskBLE(void* parameter) {
    Serial.println("[TASK] BLE service started (Core 0)");
    for (;;) {
        BleService::getInstance().update();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void taskComms(void* parameter) {
    Serial.println("[TASK] Comms manager started (Core 1)");
    for (;;) {
        PhoneBridge::getInstance().update();
        SystemMonitor::getInstance().update();

        // If SOS is active, periodically push live updates
        if (SafetyEngine::getInstance().isSOSActive()) {
            static uint32_t s_lastLiveUpdate = 0;
            if (millis() - s_lastLiveUpdate >= SOS_LIVE_UPDATE_INTERVAL_MS) {
                s_lastLiveUpdate = millis();
                SensorManager& sensors = SensorManager::getInstance();
                char lat[16], lon[16];
                snprintf(lat, sizeof(lat), "%.6f", sensors.getLatitude());
                snprintf(lon, sizeof(lon), "%.6f", sensors.getLongitude());

                String phone = Storage::getInstance().getString(NVS_KEY_PHONE, DEFAULT_EMERGENCY_PHONE);
                if (phone.length() == 0) phone = DEFAULT_EMERGENCY_PHONE;

                SOSRouting::getInstance().sendLiveUpdate(
                    phone.c_str(),
                    SafetyEngine::getInstance().getSOSManager().getSessionId(),
                    lat, lon,
                    sensors.getBPM()
                );
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Set baseline system RTC from compile timestamp (__DATE__, __TIME__) in IST (+5:30)
// Guarantees system clock is immediately valid and within seconds of real time
// even before or without WiFi NTP sync, ensuring uptime is never shown.
static void initCompileTimeBaseline() {
    setenv("TZ", "IST-5:30", 1);
    tzset();

    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    char s_month[5];
    int month = 0, day = 1, year = 2026;
    int hour = 0, min = 0, sec = 0;

    if (sscanf(__DATE__, "%s %d %d", s_month, &day, &year) == 3 &&
        sscanf(__TIME__, "%d:%d:%d", &hour, &min, &sec) == 3) {
        for (int i = 0; i < 12; i++) {
            if (strncmp(s_month, months[i], 3) == 0) {
                month = i;
                break;
            }
        }
        struct tm t = {};
        t.tm_year = year - 1900;
        t.tm_mon = month;
        t.tm_mday = day;
        t.tm_hour = hour;
        t.tm_min = min;
        t.tm_sec = sec;
        t.tm_isdst = 0;
        time_t compileEpoch = mktime(&t);
        if (compileEpoch > 0) {
            struct timeval tv = { .tv_sec = compileEpoch, .tv_usec = 0 };
            settimeofday(&tv, nullptr);
            Serial.printf("[RTC] Baseline compile time: %02d:%02d:%02d %02d/%02d/%04d (IST)\n",
                          hour, min, sec, day, month + 1, year);
        }
    }
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("╔══════════════════════════════════════════╗");
    Serial.println("║  Suraksha+ Watch OS v" WATCH_OS_VERSION "               ║");
    Serial.println("║  LilyGo T-Watch S3 + Seeed HR Sensor    ║");
    Serial.println("╚══════════════════════════════════════════╝");
    Serial.println();

    // --- Global I2C mutex ---
    g_i2cMutex = xSemaphoreCreateMutex();

    // --- 1. Platform Init ---
    Serial.println("[BOOT] 1/7 Platform...");
    initCompileTimeBaseline();
    Storage::getInstance().init();
    Watchdog::getInstance().init();
    PowerManager::getInstance().init();
    SystemMonitor::getInstance().init();

    // --- 2. Hardware Init (I2C & PMU) ---
    Serial.println("[BOOT] 2/7 Hardware (I2C & PMU)...");
    Wire.begin(PIN_SENSOR_SDA, PIN_SENSOR_SCL);
    Wire.setClock(I2C_SPEED);
    Wire.setTimeOut(50); // Prevent infinite bus freeze on unACKed transactions

    // Power up display logic and backlight rails via AXP2101
    initPMU();

    // Enable TFT backlight
#if defined(PIN_TFT_BL) && PIN_TFT_BL >= 0
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);
#endif

    // --- 3. Driver Init ---
    Serial.println("[BOOT] 3/7 Drivers...");
    DisplayDriver::getInstance().init();
    Vibration::getInstance().init();

    // --- 4. NTP Time Sync (IST UTC+5:30) via WiFi ---
    Serial.println("[BOOT] 4/8 NTP Time Sync (IST +5:30)...");

    // Display boot splash on watch
    auto* canvas = DisplayDriver::getInstance().getCanvas();
    if (canvas) {
        canvas->fillScreen(TFT_BLACK);
        canvas->setTextDatum(MC_DATUM);
        canvas->setTextColor(0x07FF, TFT_BLACK);
        canvas->drawString("SURAKSHA+ OS", 120, 50, 4);
        canvas->setTextColor(TFT_WHITE, TFT_BLACK);
        canvas->drawString("Connecting WiFi...", 120, 105, 2);
        canvas->setTextColor(0x07FF, TFT_BLACK);
        canvas->drawString(WIFI_SSID, 120, 130, 2);
        canvas->setTextColor(TFT_DARKGREY, TFT_BLACK);
        canvas->drawString("Syncing IST Time...", 120, 165, 2);
        DisplayDriver::getInstance().flush();
    }

    if (WifiManager::getInstance().connect(WIFI_CONNECT_TIMEOUT_MS)) {
        bool synced = WifiManager::getInstance().syncTime();
        // Keep WiFi connected for instant SOS dispatch & live audio streaming
        // (WifiManager::getInstance().disconnect() removed so watch remains online)

        if (canvas) {
            canvas->fillScreen(TFT_BLACK);
            canvas->setTextDatum(MC_DATUM);
            canvas->setTextColor(TFT_GREEN, TFT_BLACK);
            canvas->drawString("SURAKSHA+ OS", 120, 50, 4);
            canvas->setTextColor(TFT_WHITE, TFT_BLACK);
            canvas->drawString(synced ? "Time Synced (IST)" : "WiFi Connected", 120, 105, 2);

            time_t now = time(nullptr);
            struct tm* t = localtime(&now);
            if (t) {
                int h12 = t->tm_hour % 12;
                if (h12 == 0) h12 = 12;
                char buf[32];
                snprintf(buf, sizeof(buf), "%02d:%02d %s", h12, t->tm_min, (t->tm_hour >= 12) ? "PM" : "AM");
                canvas->setTextColor(0x07FF, TFT_BLACK);
                canvas->drawString(buf, 120, 138, 4);
            }
            DisplayDriver::getInstance().flush();
            delay(800);
        }
    } else {
        Serial.println("[BOOT] WiFi offline. Continuing with baseline clock.");
        setenv("TZ", "IST-5:30", 1);
        tzset();
        if (canvas) {
            canvas->fillScreen(TFT_BLACK);
            canvas->setTextDatum(MC_DATUM);
            canvas->setTextColor(0x07FF, TFT_BLACK);
            canvas->drawString("SURAKSHA+ OS", 120, 50, 4);
            canvas->setTextColor(TFT_YELLOW, TFT_BLACK);
            canvas->drawString("WiFi Offline", 120, 95, 2);
            bool found = WifiManager::getInstance().wasSSIDFound();
            if (!found) {
                canvas->drawString("Hotspot Not Found", 120, 118, 2);
                canvas->setTextColor(TFT_WHITE, TFT_BLACK);
                canvas->drawString("Enable 2.4GHz on Phone", 120, 140, 2);
            } else {
                canvas->drawString("Hotspot Detected", 120, 118, 2);
                canvas->setTextColor(TFT_WHITE, TFT_BLACK);
                canvas->drawString("Check Password / DHCP", 120, 140, 2);
            }
            canvas->setTextColor(TFT_DARKGREY, TFT_BLACK);
            canvas->drawString("Starting Watch...", 120, 170, 2);
            DisplayDriver::getInstance().flush();
            delay(2000);
        }
    }

    // --- 5. ESP-NOW Receiver Init (on ESPNOW_WIFI_CHANNEL) ---
    Serial.println("[BOOT] 5/8 ESP-NOW receiver...");
    ESPNowReceiver::getInstance().init();

    // --- 6. Sensor Init ---
    Serial.println("[BOOT] 6/8 Sensors...");
    SensorManager::getInstance().init();

    // --- 7. Safety Engine Init ---
    Serial.println("[BOOT] 7/8 Safety engine...");
    SafetyEngine::getInstance().init();
    SafetyEngine::getInstance().setEventCallback(onSafetyEvent);

    // --- 8. Communication Init ---
    Serial.println("[BOOT] 8/8 Communication...");
    BleService::getInstance().init();
    BleProtocol::getInstance().init();
    PhoneBridge::getInstance().init();

    // --- UI Init ---
    UIManager::getInstance().init();

    // --- Start FreeRTOS Tasks ---
    Serial.println("[BOOT] Starting FreeRTOS tasks...");

    xTaskCreatePinnedToCore(taskSafety, "safety",
        TASK_STACK_SAFETY, NULL, TASK_PRIORITY_SAFETY, &s_taskSafety, TASK_CORE_SAFETY);

    xTaskCreatePinnedToCore(taskSensor, "sensor",
        TASK_STACK_SENSOR, NULL, TASK_PRIORITY_SENSOR, &s_taskSensor, TASK_CORE_SENSOR);

    xTaskCreatePinnedToCore(taskUI, "ui",
        TASK_STACK_UI, NULL, TASK_PRIORITY_UI, &s_taskUI, TASK_CORE_UI);

    xTaskCreatePinnedToCore(taskBLE, "ble",
        TASK_STACK_BLE, NULL, TASK_PRIORITY_BLE, NULL, TASK_CORE_BLE);

    xTaskCreatePinnedToCore(taskComms, "comms",
        TASK_STACK_COMMS, NULL, TASK_PRIORITY_COMMS, NULL, TASK_CORE_COMMS);

    // Register tasks with watchdog
    Watchdog::getInstance().registerTask("safety", s_taskSafety, 5000);
    Watchdog::getInstance().registerTask("sensor", s_taskSensor, 5000);
    Watchdog::getInstance().registerTask("ui", s_taskUI, 5000);

    Serial.println();
    Serial.printf("[BOOT] ✅ System ready! Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("[BOOT] PSRAM: %d bytes free\n", ESP.getFreePsram());
    Serial.println("[BOOT] Waiting for HR sensor module via ESP-NOW...");
    Serial.println();

    // Boot vibration
    Vibration::getInstance().play(HapticPattern::SUCCESS);
}

// ============================================================================
// LOOP (mostly idle — FreeRTOS tasks handle everything)
// ============================================================================
void loop() {
    // Non-blocking vibration pattern update
    Vibration::getInstance().update();

    // Serial debug commands
    if (Serial.available()) {
        char cmd = Serial.read();
        switch (cmd) {
            case 's':
                Serial.println("[DEBUG] Manual SOS trigger");
                SafetyEngine::getInstance().triggerSOS("MANUAL");
                break;
            case 'x':
                Serial.println("[DEBUG] Stop SOS");
                SafetyEngine::getInstance().stopSOS();
                break;
            case 'a':
                Serial.println("[DEBUG] Toggle activity mode");
                if (SafetyEngine::getInstance().isActivityMode()) {
                    SafetyEngine::getInstance().stopActivityMode();
                } else {
                    SafetyEngine::getInstance().startActivityMode(300);
                }
                break;
            case 'i': {
                Serial.println("[DEBUG] System Status:");
                Serial.printf("  Heap: %d | PSRAM: %d\n",
                              ESP.getFreeHeap(), ESP.getFreePsram());
                auto& sm = SensorManager::getInstance();
                Serial.printf("  HR: BPM=%.1f SpO2=%.0f%% Online=%s RSSI=%d\n",
                              sm.getBPM(), sm.getSpO2(),
                              sm.isHRSensorOnline() ? "YES" : "NO",
                              sm.getHRSensorRSSI());
                Serial.printf("  IMU: Motion=%.2f Steps=%d\n",
                              sm.getMotionScore(), sm.getSteps());
                Serial.printf("  Battery: %d%% Charging=%s\n",
                              sm.getBatteryPercent(),
                              sm.isBatteryCharging() ? "YES" : "NO");
                auto& espnow = ESPNowReceiver::getInstance();
                Serial.printf("  ESP-NOW: Packets=%d\n",
                              espnow.getPacketsReceived());
                break;
            }
            case 'r':
                Serial.println("[DEBUG] Restarting...");
                delay(100);
                ESP.restart();
                break;
        }
    }

    delay(50);
}
