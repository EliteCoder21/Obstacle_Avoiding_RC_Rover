const int rxPin = 6;

void setup() {
  pinMode(rxPin, INPUT);
  Serial.begin(9600);

  Serial.println("Hello!");
}

unsigned long pulseIn2(int pin, int state, unsigned long timeout = 100000) {
  // pulseIn but works better on ESP32
  int lastState = digitalRead(pin);

  Serial.println(lastState);

  unsigned long start = micros();
  
  while (digitalRead(pin) == lastState) {
    if (micros() - start > timeout) return 0;
  }

  unsigned long begin = micros();
  while (digitalRead(pin) == state) {
    if (micros() - begin > timeout) return 0;
  }

  return micros() - begin;
}

void loop() {
  // Wait for long leading burst
  unsigned long p = pulseIn2(rxPin, LOW);
  if (p < 8000) return;   // not a valid lead pulse

  // Wait for space
  p = pulseIn2(rxPin, HIGH);
  if (p < 4000) return;

  // Read 8 data bits
  byte value = 0;

  for (int i = 0; i < 8; i++) {
    unsigned long burst = pulseIn2(rxPin, LOW);
    unsigned long space = pulseIn2(rxPin, HIGH);

    if (space > 1000) value |= (1 << i);  // bit=1
  }

  Serial.print("Got: ");
  Serial.println((char)value);
}
