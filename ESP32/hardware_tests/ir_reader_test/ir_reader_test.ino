/**
 * @file ir_reader_test.ino
 * @brief IR receiver test for decoding IR remote signals.
 *
 * Tests the IR receiver by decoding standard IR remote control signals.
 * Uses a custom pulseIn implementation optimized for ESP32 to handle
 * the timing requirements of IR signal decoding.
 *
 * @author Aaryan Pawar, Asaf Iron-Jobes
 * @date December 2024
 * @course CSE 474 - Embedded Systems
 */

/** GPIO pin connected to IR receiver output */
const int rxPin = 6;

/**
 * @brief Arduino setup function.
 *
 * Initializes the IR receiver pin as input and starts serial communication.
 */
void setup() {
  pinMode(rxPin, INPUT);
  Serial.begin(9600);
  Serial.println("Hello!");
}

/**
 * @brief Custom pulseIn implementation optimized for ESP32.
 *
 * Measures the duration of a pulse on a specified pin with improved
 * reliability on ESP32 microcontrollers. Handles both HIGH and LOW
 * pulse states.
 *
 * @param pin GPIO pin to read.
 * @param state Pulse state to measure (HIGH or LOW).
 * @param timeout Maximum wait time in microseconds (default 100ms).
 * @return Pulse duration in microseconds, or 0 on timeout.
 */
unsigned long pulseIn2(int pin, int state, unsigned long timeout = 100000) {
  int lastState = digitalRead(pin);
  Serial.println(lastState);

  unsigned long start = micros();

  /** Wait for initial state change */
  while (digitalRead(pin) == lastState) {
    if (micros() - start > timeout) return 0;
  }

  /** Measure pulse duration */
  unsigned long begin = micros();
  while (digitalRead(pin) == state) {
    if (micros() - begin > timeout) return 0;
  }

  return micros() - begin;
}

/**
 * @brief Main loop that continuously decodes IR remote signals.
 *
 * Implements a basic NEC-compatible IR decoder that:
 * 1. Waits for a valid leading pulse (>= 8ms LOW)
 * 2. Validates the leading space (>= 4ms HIGH)
 * 3. Reads 8 data bits using burst/space timing
 * 4. Outputs decoded character to serial
 */
void loop() {
  /** Wait for leading burst (>= 8ms LOW) */
  unsigned long p = pulseIn2(rxPin, LOW);
  if (p < 8000) return;

  /** Wait for leading space (>= 4ms HIGH) */
  p = pulseIn2(rxPin, HIGH);
  if (p < 4000) return;

  /** Decode 8 data bits */
  byte value = 0;

  for (int i = 0; i < 8; i++) {
    unsigned long burst = pulseIn2(rxPin, LOW);
    unsigned long space = pulseIn2(rxPin, HIGH);

    /** Long space indicates bit = 1 */
    if (space > 1000) value |= (1 << i);
  }

  Serial.print("Got: ");
  Serial.println((char)value);
}
