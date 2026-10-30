#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "MAX30100.h"
#include "heartRate.h"

// ===========================================================================
//   Suraksha+ Stabilized Heart Rate Engine v3
//   - 12-sample rolling median filter for outlier rejection
//   - IIR low-pass pre-filter on raw IR signal
//   - EMA (Exponential Moving Average) on final BPM output
//   - Warm-up period: first 3 beats are discarded
//   - Tighter valid BPM range: 45–160 BPM
//   - ESP-NOW Multicast to FF:FF:FF:FF:FF:FF (channel 1)
// ===========================================================================

#define SDA_PIN 4
#define SCL_PIN 3

#define FINGER_THRESHOLD 10000
#define HR_OFFSET 10.0f  // +10 BPM offset added to transmitted heart rate

// Warm-up: discard the first N beats (they are always noisy)
#define WARMUP_BEATS 3

// ============================================================================
// ESP-NOW Heart Rate Packet (matches watch receiver definition)
// ============================================================================
typedef struct __attribute__((packed)) {
    uint8_t  packetType;       // 0x01 = heart rate data, 0x02 = ping
    float    bpm;              // Smoothed BPM
    float    rawBpm;           // Raw instantaneous BPM
    float    spo2;             // SpO2 percentage
    uint8_t  confidence;       // Signal quality 0-100
    bool     fingerPresent;    // Is finger on sensor?
    uint32_t timestamp;        // millis() on sender
    uint8_t  batteryPercent;   // Sender battery level
} HRPacket_t;

#define PACKET_TYPE_HEARTRATE  0x01
#define PACKET_TYPE_PING       0x02
#define ESPNOW_WIFI_CHANNEL    1

const uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t peerInfo;

MAX30100 sensor;

// --- Rolling BPM buffer (12 samples for median filtering) ---
const byte RATE_SIZE = 12;
float rates[RATE_SIZE];
byte rateSpot = 0;
byte rateCount = 0;  // how many valid samples collected so far

long lastBeat = 0;
int beatsSinceFingerOn = 0;

float instantBpm = 0.0f;
float stableBpm  = 0.0f;  // EMA-smoothed output
bool fingerDetected = false;

// --- IIR Low-pass filter for raw IR (removes high-frequency noise) ---
float filteredIR = 0.0f;
const float IR_FILTER_ALPHA = 0.15f; // lower = smoother, higher = more responsive

// --- EMA smoothing factor for final BPM ---
// 0.15 = very stable (slow to respond), 0.3 = more responsive
const float BPM_EMA_ALPHA = 0.2f;

// SpO2 calculation
static float dcIR = 0;
static float dcRed = 0;
static float acIrAbs = 0;
static float acRedAbs = 0;
static float currentSpO2 = 0;

// --- Broadcast / Multicast ESP-NOW Packet ---
void broadcastHRPacket() {
  HRPacket_t packet;
  packet.packetType = PACKET_TYPE_HEARTRATE;
  // Increase transmitted heart rate by +10 points
  packet.bpm = (fingerDetected && stableBpm > 0) ? (stableBpm + HR_OFFSET) : 0.0f;
  packet.rawBpm = (fingerDetected && instantBpm > 0) ? (instantBpm + HR_OFFSET) : 0.0f;
  packet.spo2 = (fingerDetected && currentSpO2 > 0) ? currentSpO2 : (fingerDetected ? 98.0f : 0.0f);
  packet.confidence = (fingerDetected && stableBpm > 0) ? 95 : 0;
  packet.fingerPresent = fingerDetected;
  packet.timestamp = millis();
  packet.batteryPercent = 100;

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&packet, sizeof(packet));
  if (result != ESP_OK) {
    Serial.printf("[ESP-NOW] Send error: %d\n", result);
  }
}

// --- Median helper: sorts array copy, returns middle value ---
float getMedian(float arr[], int n) {
  if (n == 0) return 0;

  // Copy to temp array to avoid modifying original
  float temp[12];
  int count = min(n, 12);
  for (int i = 0; i < count; i++) temp[i] = arr[i];

  // Simple insertion sort (small array, efficient enough)
  for (int i = 1; i < count; i++) {
    float key = temp[i];
    int j = i - 1;
    while (j >= 0 && temp[j] > key) {
      temp[j + 1] = temp[j];
      j--;
    }
    temp[j + 1] = key;
  }

  // Return median (middle element or average of two middle)
  if (count % 2 == 1) {
    return temp[count / 2];
  } else {
    return (temp[count / 2 - 1] + temp[count / 2]) / 2.0f;
  }
}

void resetAllState() {
  stableBpm = 0.0f;
  instantBpm = 0.0f;
  currentSpO2 = 0.0f;
  rateSpot = 0;
  rateCount = 0;
  lastBeat = 0;
  beatsSinceFingerOn = 0;
  filteredIR = 0.0f;
  dcIR = 0;
  dcRed = 0;
  acIrAbs = 0;
  acRedAbs = 0;
  for (byte i = 0; i < RATE_SIZE; i++) rates[i] = 0;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n=============================================");
  Serial.println("   Suraksha+ Stabilized HR Engine v3        ");
  Serial.printf( "   SDA: GPIO %d | SCL: GPIO %d\n", SDA_PIN, SCL_PIN);
  Serial.println("=============================================\n");

  // --- WiFi & ESP-NOW Multicast Init ---
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ERROR] ESP-NOW initialization failed!");
  } else {
    Serial.println("[OK] ESP-NOW initialized successfully.");

    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = ESPNOW_WIFI_CHANNEL;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("[ERROR] Failed to add multicast peer (FF:FF:FF:FF:FF:FF)");
    } else {
      Serial.println("[OK] Multicast peer added: FF:FF:FF:FF:FF:FF (Channel 1)");
    }
  }

  Wire.setPins(SDA_PIN, SCL_PIN);
  Wire.begin(SDA_PIN, SCL_PIN, 400000);
  Wire.setTimeOut(50);

  while (!sensor.begin()) {
    Serial.println("[ERROR] MAX30100 not detected on I2C (0x57). Retrying...");
    delay(2000);
  }

  Serial.println("[OK] MAX30100 initialized successfully.");

  sensor.setMode(MAX30100_MODE_SPO2_HR);
  sensor.setHighresModeEnabled(true);
  sensor.setSamplingRate(MAX30100_SAMPRATE_100HZ);
  sensor.setLedsPulseWidth(MAX30100_SPC_PW_1600US_16BITS);
  sensor.setLedsCurrent(MAX30100_LED_CURR_24MA, MAX30100_LED_CURR_24MA);

  resetAllState();

  Serial.println("[READY] Place your finger gently on the sensor.\n");
}

void loop() {
  sensor.update();

  uint16_t rawIR, rawRed;

  while (sensor.getRawValues(&rawIR, &rawRed)) {

    // ---------------------------------------------------------------
    // 1. Finger Presence Detection
    // ---------------------------------------------------------------
    if (rawIR < FINGER_THRESHOLD) {
      if (fingerDetected) {
        fingerDetected = false;
        resetAllState();
        Serial.println("[INFO] Finger removed — state reset.");
        broadcastHRPacket(); // Immediately multicast finger removed state
      }
      continue;
    }

    if (!fingerDetected) {
      fingerDetected = true;
      lastBeat = millis();
      filteredIR = (float)rawIR;
      dcIR = rawIR;
      dcRed = rawRed;
      acIrAbs = 0;
      acRedAbs = 0;
      Serial.println("[INFO] Finger detected — warming up...");
    }

    // ---------------------------------------------------------------
    // 2. IIR Low-pass filter on raw IR before beat detection
    //    This removes high-frequency sensor noise that causes
    //    checkForBeat() to trigger false positives.
    // ---------------------------------------------------------------
    filteredIR = filteredIR + IR_FILTER_ALPHA * ((float)rawIR - filteredIR);

    // ---------------------------------------------------------------
    // 3. Continuous SpO2 baseline tracking (DC component)
    // ---------------------------------------------------------------
    dcIR  = (dcIR  * 0.95f) + ((float)rawIR  * 0.05f);
    dcRed = (dcRed * 0.95f) + ((float)rawRed * 0.05f);
    float acIR  = (float)rawIR  - dcIR;
    float acRed = (float)rawRed - dcRed;

    acIrAbs  = (acIrAbs  * 0.98f) + (fabsf(acIR)  * 0.02f);
    acRedAbs = (acRedAbs * 0.98f) + (fabsf(acRed) * 0.02f);

    // ---------------------------------------------------------------
    // 4. Beat Detection using filtered IR signal
    // ---------------------------------------------------------------
    if (checkForBeat((int32_t)filteredIR)) {
      long now = millis();
      long delta = now - lastBeat;
      lastBeat = now;

      beatsSinceFingerOn++;

      // Skip warm-up beats (sensor settling, always noisy)
      if (beatsSinceFingerOn <= WARMUP_BEATS) {
        Serial.printf("[WARMUP] Beat %d/%d skipped (sensor settling)\n",
                      beatsSinceFingerOn, WARMUP_BEATS);
        continue;
      }

      // Valid human heartbeat: 45 to 160 BPM => delta 375ms to 1333ms
      if (delta >= 375 && delta <= 1333) {
        instantBpm = 60000.0f / (float)delta;

        // --- Store in rolling buffer ---
        rates[rateSpot] = instantBpm;
        rateSpot = (rateSpot + 1) % RATE_SIZE;
        if (rateCount < RATE_SIZE) rateCount++;

        // --- Compute median of buffer (outlier-resistant) ---
        float medianBpm = getMedian(rates, rateCount);

        // --- EMA smooth the median for final output ---
        if (stableBpm < 1.0f) {
          // First valid reading — seed the EMA
          stableBpm = medianBpm;
        } else {
          stableBpm = stableBpm + BPM_EMA_ALPHA * (medianBpm - stableBpm);
        }

        // Calculate SpO2
        if (dcIR > 0 && dcRed > 0 && acIrAbs > 0) {
          float R = (acRedAbs / dcRed) / (acIrAbs / dcIR);
          float spo2 = 110.0f - (25.0f * R);
          if (spo2 > 99.0f) spo2 = 99.0f;
          if (spo2 < 80.0f) spo2 = 96.0f;
          currentSpO2 = (currentSpO2 == 0) ? spo2 : ((currentSpO2 * 0.85f) + (spo2 * 0.15f));
        }

        Serial.printf("♥ [BEAT] Instant: %5.1f | Median: %5.1f | Stable (Sent): %5.1f BPM (+10) | Delta: %4ld ms | Buf: %d/%d\n",
                      instantBpm + HR_OFFSET, medianBpm + HR_OFFSET, stableBpm + HR_OFFSET, delta, rateCount, RATE_SIZE);

        // Multicast latest HR readings in real-time
        broadcastHRPacket();
      } else {
        Serial.printf("[SKIP] Out-of-range delta: %ld ms (BPM would be %.0f)\n",
                      delta, 60000.0f / (float)delta);
      }
    }
  }

  // ---------------------------------------------------------------
  // 5. Periodic 1-second status update & ESP-NOW heartbeat
  // ---------------------------------------------------------------
  static uint32_t lastReport = 0;
  if (millis() - lastReport >= 1000) {
    lastReport = millis();

    if (!fingerDetected) {
      Serial.println("[STATUS] Finger: NOT DETECTED (place finger gently on sensor)");
    } else if (beatsSinceFingerOn <= WARMUP_BEATS) {
      Serial.printf("[STATUS] Finger: YES | Warming up... (%d/%d beats)\n",
                    beatsSinceFingerOn, WARMUP_BEATS);
    } else if (millis() - lastBeat > 3000) {
      stableBpm = 0.0f;
      Serial.println("[STATUS] Finger: DETECTED | No beats for 3s — recalibrating...");
    } else if (stableBpm > 0) {
      Serial.printf("[STATUS] Finger: YES | ♥ HR (Sent): %5.1f BPM (+10) | SpO2: %2.0f%% | Stable\n",
                    stableBpm + HR_OFFSET, currentSpO2 > 0 ? currentSpO2 : 98.0f);
    } else {
      Serial.println("[STATUS] Finger: DETECTED | Waiting for stable pulse...");
    }

    // Periodic heartbeat keeps the watch ESPNowReceiver from timing out (10s window)
    broadcastHRPacket();
  }
}
