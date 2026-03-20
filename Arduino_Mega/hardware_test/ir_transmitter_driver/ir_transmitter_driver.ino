/**
 * @file ir_transmitter_driver.ino
 * @brief IR LED transmitter driver for Arduino Mega.
 *
 * Tests IR LED transmission at 38 kHz carrier frequency using
 * software-based PWM generation on the Arduino Mega. Generates
 * the carrier wave using precise microsecond delays.
 *
 * @author Aaryan Pawar, Asaf Iron-Jobes
 * @date December 2024
 * @course CSE 474 - Embedded Systems
 */

/** GPIO pin connected to IR LED */
const int irPin = 3;

/**
 * @brief Generates a 38 kHz IR burst for the specified duration.
 *
 * Creates a 38 kHz carrier wave by toggling the output pin with
 * 13 us HIGH and 13 us LOW periods (duty cycle ~50%).
 * At 38 kHz: period = 26.3 us, so 13 us each half-cycle.
 *
 * @param duration_us Duration of the burst in microseconds.
 */
void burst38kHz(unsigned long duration_us) {
  unsigned long end = micros() + duration_us;
  while (micros() < end) {
    digitalWrite(irPin, HIGH);
    delayMicroseconds(13);
    digitalWrite(irPin, LOW);
    delayMicroseconds(13);
  }
}

/**
 * @brief Arduino setup function.
 *
 * Configures the IR pin as output for signal generation.
 */
void setup() {
  pinMode(irPin, OUTPUT);
}

/**
 * @brief Main loop - sends periodic 1ms IR bursts.
 *
 * Transmit 1ms burst of 38 kHz IR signal followed by 100ms gap.
 * Used for testing IR LED functionality and receiver pairing.
 */
void loop() {
  /** Transmit 1 ms burst of 38 kHz IR signal */
  burst38kHz(1000);

  /** Wait 100 ms before next burst */
  delay(100);
}
