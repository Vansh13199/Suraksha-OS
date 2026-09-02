// ============================================================================
// Suraksha+ Watch OS — Hardware Configuration
// Target: Seeed XIAO ESP32S3 + SH1106 128x64 OLED
// ============================================================================
#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include <Arduino.h>

// ============================================================================
// MCU Information
// ============================================================================
#define WATCH_MCU_NAME          "ESP32-S3"
#define WATCH_MCU_FREQ_MHZ      240
#define WATCH_SRAM_KB           512
#define WATCH_PSRAM_MB          8
#define WATCH_FLASH_MB          8

// ============================================================================
// Display — SH1106 128x64 OLED (I2C)
// ============================================================================
#define DISPLAY_WIDTH           128
#define DISPLAY_HEIGHT          64
#define OLED_I2C_ADDR           0x3C    // SH1106 I2C address

// OLED shares the same I2C bus as sensors (SDA=5, SCL=4)
// No separate display pins needed — all via I2C

// ============================================================================
// I2C Bus — Shared by OLED + Sensors (MAX30102 + MPU6050)
// XIAO ESP32-S3 wiring: SDA = D4/GPIO5, SCL = D5/GPIO6
// ============================================================================
#define PIN_SENSOR_SDA          5       // D4 on XIAO ESP32-S3
#define PIN_SENSOR_SCL          6       // D5 on XIAO ESP32-S3
#define I2C_SPEED               400000  // 400kHz Fast Mode

// ============================================================================
// Sensor Addresses
// ============================================================================
#define MAX30102_I2C_ADDR       0x57    // Fixed address
#define MPU6050_I2C_ADDR        0x68    // AD0 = GND

// MPU6050 Registers
#define MPU_REG_PWR_MGMT_1     0x6B
#define MPU_REG_ACCEL_XOUT_H   0x3B
#define MPU_REG_WHO_AM_I       0x75
#define MPU_REG_ACCEL_CONFIG   0x1C
#define MPU_REG_GYRO_CONFIG    0x1B
#define MPU_REG_INT_ENABLE     0x38
#define MPU_REG_INT_STATUS     0x3A
#define MPU_REG_MOT_THR        0x1F
#define MPU_REG_MOT_DUR        0x20

// ============================================================================
// Vibration Motor
// ============================================================================
#define PIN_VIBRATION           44      // GPIO44 (TX on XIAO — repurposed)
#define VIBRATION_PWM_CHANNEL   1       // LEDC channel
#define VIBRATION_PWM_FREQ      1000    // PWM frequency
#define VIBRATION_PWM_RES       8       // 8-bit resolution

// ============================================================================
// Buzzer (Optional)
// ============================================================================
#define BUZZER_ENABLED          0
#define PIN_BUZZER              -1      // Not connected by default
#define BUZZER_PWM_CHANNEL      2

// ============================================================================
// Battery Monitoring
// ============================================================================
#define PIN_BATTERY_ADC         8       // GPIO8 — ADC for battery voltage
#define BATTERY_VOLTAGE_DIVIDER 2.0f    // Voltage divider ratio (adjust per hardware)
#define BATTERY_ADC_ATTEN       ADC_ATTEN_DB_11  // Full scale ~3.3V
#define BATTERY_FULL_MV         4200    // Fully charged LiPo (mV)
#define BATTERY_EMPTY_MV        3300    // Cutoff voltage (mV)
#define BATTERY_CRITICAL_PCT    5       // Critical battery threshold (%)
#define BATTERY_LOW_PCT         20      // Low battery threshold (%)

// ============================================================================
// Button
// ============================================================================
#define PIN_BOOT_BUTTON         0       // BOOT button (GPIO0)

// ============================================================================
// GPS (UART — Optional Hardware Module)
// ============================================================================
#define GPS_HARDWARE_ENABLED    0       // Set to 1 if hardware GPS connected
#define PIN_GPS_RX              43      // UART RX from GPS module
#define PIN_GPS_TX              -1      // Not needed (GPS is TX-only)
#define GPS_BAUD_RATE           9600
#define GPS_SERIAL              Serial1

// ============================================================================
// Power Management
// ============================================================================
#define SCREEN_TIMEOUT_DEFAULT_MS   0       // 0 = always on (no auto-off)
#define SCREEN_DIM_TIMEOUT_MS       5000    // Dim 5s before off
#define DEEP_SLEEP_TIMEOUT_MS       300000  // 5 minutes of inactivity
#define OLED_DEFAULT_CONTRAST       200     // Default OLED contrast (0-255)

// ============================================================================
// FreeRTOS Task Configuration
// ============================================================================
#define TASK_STACK_SAFETY       4096
#define TASK_STACK_SENSOR       4096
#define TASK_STACK_UI           8192
#define TASK_STACK_BLE          4096
#define TASK_STACK_COMMS        8192
#define TASK_STACK_APP          2048

#define TASK_PRIORITY_SAFETY    6       // Highest — SOS engine
#define TASK_PRIORITY_SENSOR    5       // Sensor reading
#define TASK_PRIORITY_UI        4       // U8g2 rendering
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
