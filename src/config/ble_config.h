// ============================================================================
// Suraksha+ Watch OS — BLE Configuration
// Backward-compatible with existing Flutter app
// ============================================================================
#ifndef BLE_CONFIG_H
#define BLE_CONFIG_H

// ============================================================================
// BLE Device Identity
// ============================================================================
#define BLE_DEVICE_NAME         "Suraksha_Watch"
#define BLE_MANUFACTURER        "Suraksha+"

// ============================================================================
// GATT Service UUIDs (Nordic UART Service — compatible with existing firmware)
// ============================================================================
#define BLE_SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_CHAR_TX_UUID        "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // Notify (Watch → Phone)
#define BLE_CHAR_RX_UUID        "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // Write  (Phone → Watch)

// ============================================================================
// BLE Connection Parameters
// ============================================================================
#define BLE_MTU_SIZE            247     // Request larger MTU for data transfer
#define BLE_CONN_INTERVAL_MIN   6       // 7.5ms (units of 1.25ms)
#define BLE_CONN_INTERVAL_MAX   12      // 15ms
#define BLE_SLAVE_LATENCY       4       // Skip up to 4 intervals
#define BLE_SUPERVISION_TIMEOUT 400     // 4 seconds (units of 10ms)

// ============================================================================
// BLE Protocol — Message Prefixes
// ============================================================================

// --- Existing commands (backward-compatible with Flutter app) ---
#define BLE_MSG_SOS_START       "SOS:START"
#define BLE_MSG_SOS_STOP        "SOS:STOP"
#define BLE_MSG_SOS_IDENTITY    "SOS:IDENTITY_MISSING"
#define BLE_MSG_ACTIVITY_ON     "ACTIVITY:1"
#define BLE_MSG_ACTIVITY_OFF    "ACTIVITY:0"
#define BLE_MSG_ACTIVITY_ENABLED  "ACTIVITY:ENABLED"
#define BLE_MSG_ACTIVITY_DISABLED "ACTIVITY:DISABLED"
#define BLE_MSG_SET_PHONE       "SET_PHONE:"

// --- New Watch OS commands ---
// Watch → Phone
#define BLE_MSG_HEALTH_DATA     "HEALTH:"       // HEALTH:bpm,spo2,steps,calories
#define BLE_MSG_BATTERY_STATUS  "BATTERY:"       // BATTERY:pct,charging
#define BLE_MSG_SENSOR_DATA     "SENSOR:"        // SENSOR:ax,ay,az,gx,gy,gz
#define BLE_MSG_GPS_REQUEST     "GPS_REQ"        // Request GPS from phone
#define BLE_MSG_FIND_PHONE      "FIND_PHONE"     // Ring the phone
#define BLE_MSG_WATCHDOG_PING   "WDOG:PING"      // Watchdog keepalive
#define BLE_MSG_FALL_DETECTED   "FALL:DETECTED"  // Fall event
#define BLE_MSG_HR_ANOMALY      "HR:ANOMALY"     // HR anomaly event
#define BLE_MSG_AI_QUERY        "AI:Q:"          // AI query to phone

// Phone → Watch
#define BLE_MSG_TIME_SYNC       "TIME:"          // TIME:epoch_seconds
#define BLE_MSG_NOTIFICATION    "NOTIF:"         // NOTIF:type:title:body
#define BLE_MSG_GPS_DATA        "GPS:"           // GPS:lat,lon,alt,speed
#define BLE_MSG_SETTINGS        "SETTINGS:"      // SETTINGS:key:value
#define BLE_MSG_CONTACTS_SYNC   "CONTACTS:"      // Emergency contacts
#define BLE_MSG_AI_RESPONSE     "AI:R:"          // AI response from phone
#define BLE_MSG_ACTIVITY_CMD    "ACTIVITY:"      // Activity mode command
#define BLE_MSG_SOS_CMD         "SOS:"           // SOS commands
#define BLE_MSG_PHONE_ACK       "ACK"            // General acknowledgment
#define BLE_MSG_DND_MODE        "DND:"           // Do Not Disturb

// ============================================================================
// BLE Advertising Parameters
// ============================================================================
#define BLE_ADV_INTERVAL_MS     100     // Advertising interval
#define BLE_SCAN_RESPONSE       true    // Enable scan response

// ============================================================================
// BLE Reconnection
// ============================================================================
#define BLE_RECONNECT_DELAY_MS  500     // Delay before re-advertising
#define BLE_MAX_RECONNECT_ATTEMPTS 0    // 0 = unlimited

#endif // BLE_CONFIG_H
