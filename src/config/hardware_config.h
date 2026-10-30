// ============================================================================
// Suraksha+ Watch OS — Hardware Configuration
// Target: watch
// ============================================================================
#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

extern SemaphoreHandle_t g_i2cMutex;

// ============================================================================
// MCU Information
// ============================================================================
#define WATCH_MCU_NAME          "ESP32-S3"
#define WATCH_MCU_FREQ_MHZ      240
#define WATCH_SRAM_KB           512
#define WATCH_PSRAM_MB          8
#define WATCH_FLASH_MB          16

// ============================================================================
// Display — ST7789V3 240x240 TFT (SPI) — LilyGo T-Watch S3
// ============================================================================
#define DISPLAY_WIDTH           240
#define DISPLAY_HEIGHT          240

// SPI pins (defined as build flags for TFT_eSPI, listed here for reference)
// TFT_MOSI  = GPIO 13
// TFT_SCLK  = GPIO 18
// TFT_CS    = GPIO 12
// TFT_DC    = GPIO 38
// TFT_RST   = -1
#define PIN_TFT_BL              45      // Backlight control

// ============================================================================
// I2C Bus 1 — PMU + RTC + Accelerometer + Haptic Driver
// LilyGo T-Watch S3: SDA = GPIO10, SCL = GPIO11
// ============================================================================
#define PIN_SENSOR_SDA          10      // Main I2C SDA
#define PIN_SENSOR_SCL          11      // Main I2C SCL
#define I2C_SPEED               400000  // 400kHz Fast Mode

// ============================================================================
// I2C Bus 2 — Touch Panel (FT6336U)
// ============================================================================
#define PIN_TOUCH_SDA           39
#define PIN_TOUCH_SCL           40
#define PIN_TOUCH_INT           16      // Touch interrupt
#define PIN_TOUCH               PIN_TOUCH_INT // Generic touch pin for gesture detector

// ============================================================================
// Onboard Peripherals (on Main I2C Bus)
// ============================================================================
// AXP2101 Power Management Unit
#define AXP2101_I2C_ADDR        0x34
#define PIN_PMU_IRQ             21      // PMU interrupt

// PCF8563 Real-Time Clock
#define PCF8563_I2C_ADDR        0x51

// BMA423/BMA456 Accelerometer / MPU6050 Fallback
#define BMA4XX_I2C_ADDR         0x19
#define PIN_BMA_IRQ             14      // Accelerometer interrupt

// MPU6050 registers
#define MPU6050_I2C_ADDR        0x68
#define MPU_REG_PWR_MGMT_1      0x6B
#define MPU_REG_WHO_AM_I        0x75
#define MPU_REG_ACCEL_CONFIG    0x1C
#define MPU_REG_GYRO_CONFIG     0x1B
#define MPU_REG_ACCEL_XOUT_H    0x3B

// DRV2605 Haptic Motor Driver
#define DRV2605_I2C_ADDR        0x5A

// ============================================================================
// NOTE: Heart Rate Sensor (MAX30100) is NOT on the watch.
// It is on the external Seeed XIAO ESP32S3 module.
// HR data arrives via ESP-NOW (see src/comms/espnow_receiver.h)
// ============================================================================

// ============================================================================
// Audio — MAX98357A I2S Amplifier
// ============================================================================
#define PIN_I2S_BCK             9
#define PIN_I2S_WS              8
#define PIN_I2S_DOUT            18

// ============================================================================
// Microphone — SPM1423 PDM
// ============================================================================
#define PIN_MIC_SCK             44
#define PIN_MIC_DATA            47

// ============================================================================
// IR Transmitter
// ============================================================================
#define PIN_IR_TX               2

// ============================================================================
// Vibration Motor (via DRV2605 haptic driver on I2C)
// ============================================================================
// On the T-Watch S3, the vibration motor is controlled via the DRV2605
// I2C haptic driver, not a direct GPIO pin.
#define VIBRATION_USE_DRV2605   1
#define PIN_VIBRATION           -1      // Not directly wired
#define VIBRATION_PWM_CHANNEL   1       // Unused when DRV2605 active
#define VIBRATION_PWM_FREQ      1000
#define VIBRATION_PWM_RES       8

// ============================================================================
// Buzzer (Not available on T-Watch S3 — use I2S speaker instead)
// ============================================================================
#define BUZZER_ENABLED          0
#define PIN_BUZZER              -1
#define BUZZER_PWM_CHANNEL      2

// ============================================================================
// Battery Monitoring (via AXP2101 PMU — no direct ADC needed)
// ============================================================================
// Battery data is read from AXP2101 via I2C, not raw ADC
#define BATTERY_USE_PMU         1
#define PIN_BATTERY_ADC         -1
#define BATTERY_VOLTAGE_DIVIDER 1.0f    // Not used (PMU reads directly)
#define BATTERY_ADC_ATTEN       ADC_ATTEN_DB_11
#define BATTERY_FULL_MV         4200
#define BATTERY_EMPTY_MV        3300
#define BATTERY_CRITICAL_PCT    5
#define BATTERY_LOW_PCT         20

// ============================================================================
// Button
// ============================================================================
#define PIN_BOOT_BUTTON         0       // BOOT button (GPIO0)
// Power button is managed by AXP2101 PMU (2s hold = ON, 6s = OFF)

// ============================================================================
// GPS (No hardware GPS — uses phone BLE GPS or ESP-NOW sensor GPS)
// ============================================================================
#define GPS_HARDWARE_ENABLED    0
#define PIN_GPS_RX              -1
#define PIN_GPS_TX              -1
#define GPS_BAUD_RATE           9600
#define GPS_SERIAL              Serial1

// ============================================================================
// Power Management
// ============================================================================
#define SCREEN_TIMEOUT_DEFAULT_MS   30000   // 30 seconds to screen off
#define SCREEN_DIM_TIMEOUT_MS       5000    // Dim 5s before off
#define DEEP_SLEEP_TIMEOUT_MS       300000  // 5 minutes of inactivity
#define OLED_DEFAULT_CONTRAST       200     // Not used for TFT

// ============================================================================
// FreeRTOS Task Configuration
// ============================================================================
#define TASK_STACK_SAFETY       10240   // Safety engine
#define TASK_STACK_SENSOR       4096    // Sensor reading (now mainly ESP-NOW RX)
#define TASK_STACK_UI           8192    // TFT rendering
#define TASK_STACK_BLE          4096    // BLE communication
#define TASK_STACK_COMMS        8192    // WiFi/HTTP
#define TASK_STACK_APP          2048    // Background apps

#define TASK_PRIORITY_SAFETY    6       // Highest — SOS engine
#define TASK_PRIORITY_SENSOR    5       // Sensor reading
#define TASK_PRIORITY_UI        4       // Display rendering
#define TASK_PRIORITY_BLE       3       // Bluetooth
#define TASK_PRIORITY_COMMS     2       // WiFi/HTTP
#define TASK_PRIORITY_APP       1       // Background apps

#define TASK_CORE_SAFETY        0
#define TASK_CORE_SENSOR        0
#define TASK_CORE_UI            1
#define TASK_CORE_BLE           0
#define TASK_CORE_COMMS         1
#define TASK_CORE_APP           1

#endif // HARDWARE_CONFIG_H
