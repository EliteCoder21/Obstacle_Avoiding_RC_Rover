const int irPin = 3;

void burst38kHz(unsigned long duration_us) {
  unsigned long end = micros() + duration_us;
  while (micros() < end) {
    digitalWrite(irPin, HIGH);
    delayMicroseconds(13);
    digitalWrite(irPin, LOW);
    delayMicroseconds(13);
  }
}

void setup() {
  pinMode(irPin, OUTPUT);
}

void loop() {
  // send 1 ms burst
  burst38kHz(1000);
  delay(100);   // gap
}
