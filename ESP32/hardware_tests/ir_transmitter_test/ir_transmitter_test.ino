/**
 * @file ir_transmitter_test.ino
 * @brief IR LED transmitter test using ESP32-S3.
 *
 * Tests IR LED transmission at 38 kHz carrier frequency using the
 * ESP32's LEDC (LED Controller) PWM peripheral. This frequency is
 * standard for IR remote control protocols.
 *
 * @author Aaryan Pawar, Asaf Iron-Jobes
 * @date December 2024
 * @course CSE 474 - Embedded Systems
 */

/** GPIO pin connected to IR LED anode */
#define IR_LED_PIN 3

/** LEDC PWM channel to use */
#define PWM_CHANNEL 0

/** IR carrier frequency (38 kHz standard for IR remotes) */
#define PWM_FREQ 38000

/** PWM resolution in bits (8-bit = 0-255) */
#define PWM_RES 8

/**
 * @brief Arduino setup function.
 *
 * Configures the LEDC peripheral to generate a 38 kHz PWM signal
 * at 50% duty cycle for continuous IR transmission testing.
 */
void setup() {
  /** Attach IR_LED_PIN to LEDC channel with 38 kHz frequency */
  ledcAttach(IR_LED_PIN, PWM_FREQ, PWM_RES);

  /** Set duty cycle to 50% (128/255) */
  ledcWrite(IR_LED_PIN, 128);
}

/**
 * @brief Main loop - transmission runs continuously in setup.
 */
void loop() {
}
