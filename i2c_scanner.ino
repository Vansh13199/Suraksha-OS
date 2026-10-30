
#define TEST_OUT 1
#define TEST_IN  2

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(TEST_OUT, OUTPUT);
  pinMode(TEST_IN, INPUT);

  Serial.println("GPIO loopback test started");
  Serial.println("GPIO 1 must be connected to GPIO 2");
}

void loop() {
  digitalWrite(TEST_OUT, HIGH);
  delay(10);
  int highRead = digitalRead(TEST_IN);

  digitalWrite(TEST_OUT, LOW);
  delay(10);
  int lowRead = digitalRead(TEST_IN);

  if (highRead == HIGH && lowRead == LOW) {
    Serial.println("[PASS] GPIO 1 and GPIO 2 respond correctly");
  } else {
    Serial.printf(
      "[FAIL] HIGH read=%d, LOW read=%d\n",
      highRead, lowRead
    );
  }

  delay(1000);
}
