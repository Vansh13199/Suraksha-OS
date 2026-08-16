// ============================================================================
// Suraksha+ Watch OS — Application Configuration
// ============================================================================
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// ============================================================================
// Version
// ============================================================================
#define WATCH_OS_VERSION        "1.0.0"
#define WATCH_OS_NAME           "Suraksha+ Watch OS"
#define WATCH_OS_BUILD          __DATE__ " " __TIME__

// ============================================================================
// SOS Configuration
// ============================================================================
#define SOS_COOLDOWN_MS             10000   // Minimum time between SOS triggers
#define SOS_AUTO_COOLDOWN_MS        60000   // Auto-SOS (HR/fall) cooldown
#define SOS_LIVE_UPDATE_INTERVAL_MS 5000    // Live location update interval
#define SOS_CANCEL_HOLD_MS          5000    // Hold duration to cancel SOS
#define SOS_FALL_COUNTDOWN_MS       10000   // Countdown before fall-triggered SOS

// ============================================================================
// AWS API Endpoints
// ============================================================================
#define AWS_SOS_TRIGGER_URL     "https://3argzpovmc.execute-api.ap-south-1.amazonaws.com/prod/sos/trigger"
#define AWS_SOS_STOP_URL        "https://3argzpovmc.execute-api.ap-south-1.amazonaws.com/prod/sos/stop"
#define AWS_HTTP_TIMEOUT_MS     15000
#define AWS_TLS_RX_BUFFER       4096
#define AWS_TLS_TX_BUFFER       2048

// ============================================================================
// Gesture Detection
// ============================================================================
#define GESTURE_TAP_IDLE_MS         450     // Max gap between taps
#define GESTURE_LONG_PRESS_MS       5000    // Long press duration
#define GESTURE_DEBOUNCE_MS         8       // Touch debounce time
#define GESTURE_TAP_SOS             2       // 2 taps → SOS
#define GESTURE_TAP_ACTIVITY_OFF    3       // 3 taps → Activity OFF
#define GESTURE_TAP_ACTIVITY_ON     4       // 4 taps → Activity ON

// ============================================================================
// Heart Rate Monitoring
// ============================================================================
#define HR_SAMPLE_WINDOW            8       // Rolling average window size
#define HR_MIN_VALID_BPM            35.0f   // Minimum valid BPM
#define HR_MAX_VALID_BPM            200.0f  // Maximum valid BPM
#define HR_ANOMALY_LOW_BPM          40.0f   // Below this = anomaly
#define HR_ANOMALY_HIGH_BPM         140.0f  // Above this = anomaly
#define HR_ANOMALY_CONFIRM_COUNT    5       // Consecutive anomalies to trigger
#define HR_IR_THRESHOLD             20000   // Finger-on-sensor threshold
#define HR_BEAT_MIN_INTERVAL_MS     250     // Minimum beat interval
#define HR_BEAT_MAX_INTERVAL_MS     2000    // Maximum beat interval

// ============================================================================
// IMU / Motion Detection
// ============================================================================
#define IMU_POLL_RATE_HZ            50      // Normal polling rate
#define IMU_POLL_RATE_LP_HZ         10      // Low-power polling rate
#define IMU_MOTION_SMOOTHING        0.85f   // EMA smoothing factor
#define IMU_FALL_IMPACT_G           3.0f    // Impact threshold for fall detection
#define IMU_FALL_IMPACT_DURATION_MS 50      // Minimum impact duration
#define IMU_FALL_STILL_THRESHOLD    0.1f    // Motion score for "still"
#define IMU_FALL_STILL_DURATION_MS  5000    // Still duration after impact
#define IMU_STEP_THRESHOLD_G        1.2f    // Step detection threshold

// ============================================================================
// Battery Thresholds
// ============================================================================
#define BATTERY_UPDATE_INTERVAL_MS  30000   // Battery check interval (30s)
#define BATTERY_LP_UPDATE_MS        60000   // Low-power mode interval (60s)

// ============================================================================
// Activity Mode
// ============================================================================
#define ACTIVITY_DEFAULT_DURATION_S 1800    // Default 30 minutes
#define ACTIVITY_MAX_DURATION_S     7200    // Maximum 2 hours
#define ACTIVITY_SENSOR_RATE_HZ     50      // Enhanced sensor rate during activity

// ============================================================================
// Notification
// ============================================================================
#define NOTIF_MAX_COUNT             20      // Max stored notifications
#define NOTIF_POPUP_DURATION_MS     5000    // Popup display time
#define NOTIF_VIBRATE_DURATION_MS   200     // Vibration for notification

// ============================================================================
// Display / UI
// ============================================================================
#define UI_FPS                      30      // Target frame rate
#define UI_TICK_MS                  (1000 / UI_FPS)
#define UI_ANIMATION_SPEED_MS       300     // Screen transition duration
#define SCREEN_TIMEOUT_OPTIONS      {5000, 10000, 15000, 30000}  // ms

// ============================================================================
// Storage Keys (NVS Namespace: "suraksha")
// ============================================================================
#define NVS_NAMESPACE               "suraksha"
#define NVS_KEY_PHONE               "phone"
#define NVS_KEY_BRIGHTNESS          "bright"
#define NVS_KEY_SCREEN_TIMEOUT      "scrn_to"
#define NVS_KEY_VIBRATION           "vib_lvl"
#define NVS_KEY_DND_MODE            "dnd"
#define NVS_KEY_POWER_SAVE          "pwr_save"
#define NVS_KEY_WATCHFACE           "wf_idx"
#define NVS_KEY_STEPS_TODAY         "steps"
#define NVS_KEY_STEP_DATE           "step_date"
#define NVS_KEY_ALARM_COUNT         "alm_cnt"
#define NVS_KEY_ALARM_PREFIX        "alm_"
#define NVS_KEY_CRASH_COUNT         "crashes"
#define NVS_KEY_TIMEZONE_OFFSET     "tz_off"

// ============================================================================
// WiFi Configuration (for direct SOS fallback)
// ============================================================================
#define WIFI_CONNECT_TIMEOUT_MS     10000
#define WIFI_MAX_RETRY              3

// ============================================================================
// Watchdog
// ============================================================================
#define WDT_TIMEOUT_S               8       // Hardware watchdog timeout
#define WDT_TASK_TIMEOUT_MS         5000    // Per-task watchdog

#endif // APP_CONFIG_H
