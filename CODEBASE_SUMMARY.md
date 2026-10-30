# Suraksha+ Watch OS — Codebase Summary

## Overview

**Suraksha+ Watch OS** is a safety-focused embedded operating system for the **Seeed XIAO ESP32S3** microcontroller with a **SH1106 128x64 OLED display**. It provides real-time health monitoring, fall detection, heart rate anomaly detection, and multi-path SOS emergency communication.

---

## Hardware Target

| Component | Specification |
|-----------|---------------|
| MCU | ESP32-S3 (Dual-core, 240 MHz, 512 KB SRAM, 8 MB PSRAM, 8 MB Flash) |
| Display | SH1106 128x64 OLED (I2C @ 0x3C) |
| Heart Rate | MAX30102 (I2C @ 0x57) |
| IMU | MPU6050 (I2C @ 0x68) |
| Vibration | GPIO44 (LEDC PWM) |
| Battery ADC | GPIO8 (Voltage divider, 2.0x ratio) |
| GPS | Optional UART (GPIO43 RX) |
| I2C Bus | Shared: SDA=GPIO5, SCL=GPIO6 @ 400kHz |

---

## Architecture

### FreeRTOS Task Layout (Dual-Core)

```
Core 0 (High-Priority):
├── Task: safety      (Priority 6, Stack 4KB)  — Safety Engine, SOS, Fall/HR detection
├── Task: sensor      (Priority 5, Stack 4KB)  — Sensor reading (HR, IMU, Battery)
└── Task: ble         (Priority 3, Stack 4KB)  — BLE communication

Core 1 (UI/Background):
├── Task: ui          (Priority 4, Stack 8KB)  — U8g2 rendering @ 30 FPS
└── Task: comms       (Priority 2, Stack 8KB)  — WiFi, HTTP, Phone Bridge, SOS routing
```

### Boot Sequence (src/main.cpp:219-319)

1. **Platform Init** — Storage (NVS), Watchdog, Power Manager, System Monitor
2. **Hardware Init** — I2C bus initialization
3. **Driver Init** — Vibration motor, Buzzer (optional)
4. **Sensor Init** — Heart Rate, IMU, Battery, GPS
5. **Safety Engine Init** — SOS Manager, Gesture Detector, Fall Detector, HR Anomaly, Activity Mode
6. **Communication Init** — BLE Service, BLE Protocol, Phone Bridge, WiFi + NTP Time Sync
7. **UI Init** — Display Driver, U8g2, Screen Manager
8. **Start FreeRTOS Tasks** — Register with watchdog supervisor

---

## Module Architecture

### 1. Configuration (`src/config/`)

| File | Purpose |
|------|---------|
| `hardware_config.h` | Pin definitions, I2C addresses, FreeRTOS task config, display params |
| `app_config.h` | SOS timing, AWS endpoints, gesture/HR/IMU thresholds, NVS keys, WiFi credentials |
| `ble_config.h` | BLE UUIDs (Nordic UART Service), message protocol prefixes, connection params |

### 2. Platform (`src/platform/`)

| Module | Description |
|--------|-------------|
| `storage.h/cpp` | NVS wrapper (Preferences) for persistent settings |
| `watchdog.h/cpp` | Dual-layer watchdog: ESP Task WDT + per-task software monitor with crash counting |
| `power_manager.h/cpp` | Sleep prevention, deep sleep entry, battery-aware power modes |
| `system_monitor.h/cpp` | Heap/PSRAM/CPU monitoring, task stack watermarks |

### 3. Drivers (`src/drivers/`)

| Module | Description |
|--------|-------------|
| `display_driver.h/cpp` | U8g2 SH1106 128x64 driver wrapper, contrast/brightness control |
| `vibration.h/cpp` | Haptic feedback via PWM (patterns: SUCCESS, ALARM, SOS_CONFIRM, NOTIFICATION, etc.) |
| `buzzer.h/cpp` | Optional buzzer support (disabled by default) |

### 4. Sensors (`src/sensors/`)

| Module | Description |
|--------|-------------|
| `sensor_manager.h/cpp` | **Orchestrator** — Power modes (ACTIVE/POWER_SAVE/SLEEP), mutex-protected data access, polling rate management |
| `heart_rate.h/cpp` | MAX30102 driver: BPM calculation, SpO2, finger detection, beat detection, rolling average (8 samples) |
| `imu.h/cpp` | MPU6050 driver: Accelerometer/gyro reading, motion score, step counting, tap detection, low-power mode |
| `battery.h/cpp` | ADC-based voltage monitoring, percentage calculation, charging detection |
| `gps.h/cpp` | Optional hardware GPS (UART) + BLE phone GPS fusion, NMEA parsing |

**Sensor Data Structures:**
```cpp
struct HeartRateData { float bpm, spo2; bool fingerPresent; uint32_t irValue; };
struct IMUData { float ax, ay, az, gx, gy, gz; float accMagnitude; float motionScore; uint32_t steps; };
struct BatteryData { float voltage; uint8_t percentage; bool isCharging; };
struct GPSData { double latitude, longitude, altitude; float speed; bool hasFix; uint32_t lastUpdate; };
```

### 5. Safety Engine (`src/safety/`)

**Central coordinator** running at highest priority (Core 0, 50Hz).

| Component | Description |
|-----------|-------------|
| `safety_engine.h/cpp` | State machine (IDLE → SOS_COUNTDOWN → SOS_ACTIVE / ACTIVITY_MODE), gesture processing, coordinates all detectors |
| `sos_manager.h/cpp` | SOS session lifecycle, UUID generation (matches existing firmware), cooldown enforcement |
| `fall_detector.h/cpp` | Impact detection (>3g) + stillness confirmation (5s < 0.1g motion score) |
| `hr_anomaly.h/cpp` | Consecutive anomaly detection (5 readings outside 40-140 BPM) |
| `gesture_detector.h/cpp` | Tap counting (2=SOS, 3=Activity OFF, 4=Activity ON), long press (5s=Stop SOS) |
| `activity_mode.h/cpp` | Timed activity mode (default 30min, max 2hr) with enhanced sensor rate |

**State Transitions:**
```
IDLE
  ├── Fall detected → SOS_COUNTDOWN (10s) → SOS_ACTIVE
  ├── HR anomaly detected → SOS_ACTIVE (immediate)
  ├── 2-tap gesture → SOS_ACTIVE
  ├── 4-tap gesture → ACTIVITY_MODE
  └── Activity timeout → IDLE

SOS_ACTIVE
  ├── Long press (5s) → IDLE
  ├── Phone command STOP → IDLE
  └── Serial 'x' command → IDLE
```

### 6. Communication (`src/comms/`)

| Module | Description |
|--------|-------------|
| `ble_service.h/cpp` | BLE GATT server (Nordic UART), connection management, MTU 247 |
| `ble_protocol.h/cpp` | Message parser/dispatcher for phone ↔ watch commands |
| `phone_bridge.h/cpp` | High-level phone communication abstraction |
| `wifi_manager.h/cpp` | WiFi connection, NTP time sync, connection management |
| `sos_routing.h/cpp` | **Multi-path SOS routing**: BLE→Phone (priority) → WiFi Direct→AWS → Store&Forward |

**BLE Message Protocol (backward-compatible):**
```
Phone → Watch:
  SET_PHONE:+91XXXXXXXXXX      — Emergency contact
  ACTIVITY:1[SECONDS:N]        — Start activity mode
  ACTIVITY:0                   — Stop activity mode
  TIME:epoch_seconds           — NTP time sync
  NOTIF:type:title:body        — Notification
  GPS:lat,lon,alt,speed        — Phone GPS
  SOS:START / SOS:STOP         — Remote SOS control
  DND:1 / DND:0                — Do Not Disturb

Watch → Phone:
  SOS:START / SOS:STOP         — SOS state
  HEALTH:bpm,spo2,steps,cal    — Health data
  BATTERY:pct,charging         — Battery status
  SENSOR:ax,ay,az,gx,gy,gz     — IMU data
  FALL:DETECTED                — Fall event
  HR:ANOMALY                   — HR anomaly event
```

**SOS Routing Priority:**
1. **BLE → Phone** (if connected) — Phone handles AWS API + GPS
2. **WiFi Direct → AWS** (if WiFi connected) — Direct HTTPS POST
3. **Store & Forward** — Queue in NVS for later (TODO)

### 7. UI (`src/ui/`)

**U8g2-based** monochrome OLED rendering at 30 FPS.

| Screen | Description |
|--------|-------------|
| `watchface_screen` | Time, date, battery, HR, steps, connection status |
| `health_screen` | Detailed health metrics (HR, SpO2, steps, calories) |
| `system_info_screen` | Heap, PSRAM, CPU, WiFi, BLE, crash count |
| `sos_screen` | Full-screen SOS overlay with countdown/session info |

**Navigation:** Auto-cycle every 5s (Watchface → Health → System Info), manual via gestures (not yet implemented), SOS overrides all.

---

## Key Features

### SOS System
- **Manual trigger**: 2-tap gesture, long press (5s to stop), serial command, phone command
- **Auto trigger**: Fall detection (impact + stillness), HR anomaly (5 consecutive readings)
- **Countdown**: 10s cancel window for fall-triggered SOS
- **Live updates**: Every 5s with location + HR via best available route
- **Multi-path**: BLE→Phone→AWS (preferred) or WiFi→AWS direct
- **Session tracking**: UUID matching existing firmware format

### Health Monitoring
- **Heart Rate**: Continuous MAX30102 reading, finger detection, BPM + SpO2
- **Activity Tracking**: Step counting via IMU, calorie estimation
- **Fall Detection**: 3g impact threshold + 5s stillness confirmation
- **HR Anomaly**: <40 or >140 BPM for 5 consecutive readings

### Power Management
- **Modes**: ACTIVE (50Hz IMU, continuous HR) / POWER_SAVE (10Hz IMU, 5s HR) / SLEEP (1Hz IMU, HR off)
- **Deep Sleep**: 5min inactivity timeout
- **Screen**: Configurable timeout (default always-on), dim before off

### Reliability
- **Watchdog**: Hardware WDT (8s) + software per-task monitoring (5s default)
- **Crash Recovery**: NVS crash counter, unclean boot detection
- **Thread Safety**: Mutex-protected sensor data access
- **Task Isolation**: Safety engine on dedicated core, survives UI/comms failures

---

## Build System

- **PlatformIO** with Espressif32 platform v6.4.0
- **Framework**: Arduino
- **Key Library**: U8g2 v2.35.9 (display)
- **Sensors**: SparkFun MAX3010x v1.1.2
- **Partition**: 8MB flash (default_8MB.csv)
- **Build Type**: Release, 240 MHz CPU

---

## NVS Storage Keys (Namespace: "suraksha")

| Key | Type | Description |
|-----|------|-------------|
| `phone` | String | Emergency contact number |
| `bright` | Int | OLED brightness (0-255) |
| `scrn_to` | Int | Screen timeout (ms) |
| `vib_lvl` | Int | Vibration intensity |
| `dnd` | Bool | Do Not Disturb mode |
| `pwr_save` | Bool | Power save mode |
| `wf_idx` | Int | Watchface index |
| `steps` | UInt32 | Daily step count |
| `step_date` | String | Step count date |
| `alm_cnt` | Int | Alarm count |
| `alm_*` | String | Alarm definitions |
| `crashes` | UInt32 | Crash counter |
| `tz_off` | Int | Timezone offset (seconds) |

---

## Debug Commands (Serial)

| Key | Action |
|-----|--------|
| `s` | Manual SOS trigger |
| `x` | Stop SOS |
| `a` | Toggle activity mode (5 min) |
| `i` | Print system status |
| `r` | Restart device |

---

## File Structure

```
src/
├── main.cpp                      # Entry point, FreeRTOS task creation
├── config/
│   ├── hardware_config.h         # Pinout, task config, display params
│   ├── app_config.h              # App constants, thresholds, NVS keys
│   └── ble_config.h              # BLE UUIDs, message protocol
├── platform/
│   ├── storage.h/cpp             # NVS wrapper
│   ├── watchdog.h/cpp            # Dual-layer watchdog
│   ├── power_manager.h/cpp       # Sleep/power management
│   └── system_monitor.h/cpp      # System health monitoring
├── drivers/
│   ├── display_driver.h/cpp      # U8g2 SH1106 driver
│   ├── vibration.h/cpp           # Haptic patterns
│   └── buzzer.h/cpp              # Optional buzzer
├── sensors/
│   ├── sensor_manager.h/cpp      # Sensor orchestrator
│   ├── heart_rate.h/cpp          # MAX30102 driver
│   ├── imu.h/cpp                 # MPU6050 driver
│   ├── battery.h/cpp             # Battery ADC
│   └── gps.h/cpp                 # GPS (HW + BLE fusion)
├── safety/
│   ├── safety_engine.h/cpp       # Central safety coordinator
│   ├── sos_manager.h/cpp         # SOS session management
│   ├── fall_detector.h/cpp       # Fall detection algorithm
│   ├── hr_anomaly.h/cpp          # HR anomaly detection
│   ├── gesture_detector.h/cpp    # Tap/long-press detection
│   └── activity_mode.h/cpp       # Timed activity mode
├── comms/
│   ├── ble_service.h/cpp         # BLE GATT server
│   ├── ble_protocol.h/cpp        # Message parser
│   ├── phone_bridge.h/cpp        # Phone communication
│   ├── wifi_manager.h/cpp        # WiFi + NTP
│   └── sos_routing.h/cpp         # Multi-path SOS routing
└── ui/
    ├── ui_manager.h/cpp          # Screen manager, auto-cycle
    └── screens/
        ├── watchface_screen.h/cpp
        ├── health_screen.h/cpp
        ├── system_info_screen.h/cpp
        └── sos_screen.h/cpp
```

---

## Development Notes

### Adding a New Sensor
1. Create `src/sensors/new_sensor.h/cpp` with `init()` and `update()` methods
2. Add data struct to `sensor_manager.h`
3. Integrate in `SensorManager::init()` and `SensorManager::update()`
4. Add thread-safe accessor methods

### Adding a New Screen
1. Create `src/ui/screens/new_screen.h/cpp` with static `draw(U8G2*)` method
2. Add screen enum to `UIManager::Screen`
3. Add to `_cycleScreens` array if auto-cycling
4. Add case in `UIManager::update()` switch statement

### Adding a BLE Command
1. Add prefix to `ble_config.h`
2. Add handler in `BleProtocol::_handleNewCommand()`
3. Dispatch in `BleProtocol::handleMessage()`

### SOS Routing Extension
1. Add route type to `SOSRoute` enum
2. Implement `_sendViaNewRoute()` in `sos_routing.cpp`
3. Add to priority logic in `getBestRoute()`

---

## Version

**Suraksha+ Watch OS v1.0.0** — Built for Seeed XIAO ESP32S3 + SH1106 OLED