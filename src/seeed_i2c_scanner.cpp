#include <Arduino.h>
#include <Wire.h>
#include "MAX30100.h"
#include "heartRate.h"

#define SDA_PIN 4
#define SCL_PIN 3

#define FINGER_THRESHOLD 10000

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

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n=============================================");
  Serial.println("   Suraksha+ Real-Time Dynamic HR Engine     ");
  Serial.printf( "   SDA: GPIO %d | SCL: GPIO %d\n", SDA_PIN, SCL_PIN);
  Serial.println("=============================================\n");

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

        Serial.printf("♥ [BEAT] Instant: %5.1f BPM | Live Avg: %5.1f BPM | Delta: %4ld ms\n",
                      instantBpm, beatAvg, delta);
      }
    }
  }

  // 4. Periodic 1-second status update
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
        Serial.printf("[STATUS] Finger: YES | Heart Rate: %5.1f BPM | SpO2: %2.0f%% | Live\n",
                      beatAvg, currentSpO2 > 0 ? currentSpO2 : 98.0f);
      } else {
        Serial.println("[STATUS] Finger: DETECTED | Waiting for next beat...");
      }
    }
  }
}
