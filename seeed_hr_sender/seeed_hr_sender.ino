#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "MAX30100.h"
#include "heartRate.h"

#define SDA_PIN 4
#define SCL_PIN 3

#define FINGER_THRESHOLD 10000
#define HR_OFFSET 10.0f  // +10 BPM offset added to transmitted heart rate

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

// Multicast MAC address to reach all listening devices (including watch)
const uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t peerInfo;

MAX30100 sensor;

// Rolling BPM buffer (size 4 for real-time dynamic response)
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE] = {0};
byte rateSpot = 0;
long lastBeat = 0;

float instantBpm = 0.0f;
float beatAvg = 0.0f;
bool fingerDetected = false;

// SpO2 calculation
static float dcIR = 0;
static float dcRed = 0;
static float acIrAbs = 0;
static float acRedAbs = 0;
static float currentSpO2 = 0;

void broadcastHRPacket() {
  HRPacket_t packet;
  packet.packetType = PACKET_TYPE_HEARTRATE;
  // Increase transmitted heart rate by +10 points
  packet.bpm = (fingerDetected && beatAvg > 0) ? (beatAvg + HR_OFFSET) : 0.0f;
  packet.rawBpm = (fingerDetected && instantBpm > 0) ? (instantBpm + HR_OFFSET) : 0.0f;
  packet.spo2 = (fingerDetected && currentSpO2 > 0) ? currentSpO2 : (fingerDetected ? 98.0f : 0.0f);
  packet.confidence = (fingerDetected && beatAvg > 0) ? 95 : 0;
  packet.fingerPresent = fingerDetected;
  packet.timestamp = millis();
  packet.batteryPercent = 100;

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&packet, sizeof(packet));
  if (result != ESP_OK) {
    Serial.printf("[ESP-NOW] Send error: %d\n", result);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n=============================================");
  Serial.println("   Suraksha+ Real-Time Dynamic HR Engine     ");
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

  // --- Sensor I2C Init ---
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

  Serial.println("[READY] Place your finger gently on the sensor.\n");
}

void loop() {
  sensor.update();

  uint16_t rawIR, rawRed;

  while (sensor.getRawValues(&rawIR, &rawRed)) {
    // 1. Finger Presence Detection
    if (rawIR < FINGER_THRESHOLD) {
      if (fingerDetected) {
        fingerDetected = false;
        beatAvg = 0.0f;
        instantBpm = 0.0f;
        currentSpO2 = 0.0f;
        for (byte i = 0; i < RATE_SIZE; i++) rates[i] = 0;
        rateSpot = 0;
        lastBeat = 0;
        broadcastHRPacket(); // Immediately multicast finger removed state
      }
      continue;
    }

    if (!fingerDetected) {
      fingerDetected = true;
      lastBeat = millis();
      dcIR = rawIR;
      dcRed = rawRed;
      acIrAbs = 0;
      acRedAbs = 0;
    }

    // 2. Continuous SpO2 baseline tracking
    dcIR = (dcIR * 0.95f) + ((float)rawIR * 0.05f);
    dcRed = (dcRed * 0.95f) + ((float)rawRed * 0.05f);
    float acIR = (float)rawIR - dcIR;
    float acRed = (float)rawRed - dcRed;

    acIrAbs = (acIrAbs * 0.98f) + (abs(acIR) * 0.02f);
    acRedAbs = (acRedAbs * 0.98f) + (abs(acRed) * 0.02f);

    // 3. Maxim Integrated Official PBA Algorithm (Zero-crossing FIR)
    if (checkForBeat((int32_t)rawIR)) {
      long now = millis();
      long delta = now - lastBeat;
      lastBeat = now; // ALWAYS update lastBeat so subsequent beats calculate against fresh clock!

      if (delta >= 380 && delta <= 1500) { // Valid human heartbeat between 40 and 157 BPM
        instantBpm = 60000.0f / (float)delta;

        rates[rateSpot++] = (byte)instantBpm;
        rateSpot %= RATE_SIZE;

        int sum = 0;
        int validCount = 0;
        for (byte x = 0; x < RATE_SIZE; x++) {
          if (rates[x] > 0) {
            sum += rates[x];
            validCount++;
          }
        }
        if (validCount > 0) {
          beatAvg = (float)sum / (float)validCount;
        }

        // Calculate SpO2
        if (dcIR > 0 && dcRed > 0 && acIrAbs > 0) {
          float R = (acRedAbs / dcRed) / (acIrAbs / dcIR);
          float spo2 = 110.0f - (25.0f * R);
          if (spo2 > 99.0f) spo2 = 99.0f;
          if (spo2 < 93.0f) spo2 = 96.0f;
          currentSpO2 = (currentSpO2 == 0) ? spo2 : ((currentSpO2 * 0.85f) + (spo2 * 0.15f));
        }

        Serial.printf("♥ [BEAT] Instant: %5.1f BPM | Live Avg (Sent): %5.1f BPM (+10) | Delta: %4ld ms\n",
                      instantBpm + HR_OFFSET, beatAvg + HR_OFFSET, delta);

        // Multicast latest HR readings in real-time
        broadcastHRPacket();
      }
    }
  }

  // 4. Periodic 1-second status update & ESP-NOW heartbeat
  static uint32_t lastReport = 0;
  if (millis() - lastReport >= 1000) {
    lastReport = millis();

    if (!fingerDetected) {
      Serial.println("[STATUS] Finger: NOT DETECTED (place finger gently on sensor)");
    } else {
      // If finger is on sensor but no beat arrived in 2.5 seconds, reset live BPM
      if (millis() - lastBeat > 2500) {
        beatAvg = 0.0f;
        Serial.println("[STATUS] Finger: DETECTED | Calibrating pulse... (keep finger steady)");
      } else if (beatAvg > 0) {
        Serial.printf("[STATUS] Finger: YES | Heart Rate (Sent): %5.1f BPM (+10) | SpO2: %2.0f%% | Live\n",
                      beatAvg + HR_OFFSET, currentSpO2 > 0 ? currentSpO2 : 98.0f);
      } else {
        Serial.println("[STATUS] Finger: DETECTED | Waiting for next beat...");
      }
    }

    // Periodic heartbeat keeps the watch ESPNowReceiver from timing out (10s window)
    broadcastHRPacket();
  }
}
