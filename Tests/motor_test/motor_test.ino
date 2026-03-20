/**
 * @file motor_test.ino
 * @brief Motor driver test for the RC rover.
 *
 * Tests basic DC motor control using PWM speed control and directional
 * switching via H-bridge logic. Validates motor driver functionality
 * before integration with the main rover system.
 *
 * @author Aaryan Pawar, Asaf Iron-Jobes
 * @date December 2024
 * @course CSE 474 - Embedded Systems
 */

#include <Arduino.h>

/** Motor A enable pin (PWM speed control) */
int enA = 11;

/** Motor A direction pin 1 */
int in1 = 10;

/** Motor A direction pin 2 */
int in2 = 9;

/** Motor B enable pin (PWM speed control) */
int enB = 14;

/** Motor B direction pin 1 */
int in3 = 13;

/** Motor B direction pin 2 */
int in4 = 12;

/** PWM frequency in Hz */
const int freq = 1000;

/** PWM channel for motor A */
const int pwmChannelA = 0;

/** PWM channel for motor B */
const int pwmChannelB = 1;

/** PWM resolution in bits (8-bit = 0-255) */
const int resolution = 8;

/**
 * @brief Arduino setup function.
 *
 * Initializes motor control pins as outputs and configures PWM channels
 * for speed control. Motors are initially stopped.
 */
void setup() {
  /** Configure motor direction control pins as outputs */
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  /** Attach enable pins to PWM channels */
  ledcAttachChannel(enA, freq, resolution, pwmChannelA);
  ledcAttachChannel(enB, freq, resolution, pwmChannelB);

  /** Initialize motors in stopped state */
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  ledcWrite(enA, 0);
  ledcWrite(enB, 0);
}

/**
 * @brief Main loop that alternates between direction and speed tests.
 */
void loop() {
  directionControl();
  delay(1000);
  speedControl();
  delay(1000);
}

/**
 * @brief Tests motor direction control by running forward and backward.
 *
 * Motors are set to full speed and directions are toggled between forward
 * and reverse to validate H-bridge direction switching.
 */
void directionControl() {
  /** Set motors to maximum speed */
  ledcWrite(enA, 255);
  ledcWrite(enB, 255);

  /** Run motors forward for 2 seconds */
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  delay(2000);

  /** Reverse motor direction for 2 seconds */
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  delay(2000);

  /** Stop motors */
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

/**
 * @brief Tests motor speed control by ramping speed up and down.
 *
 * Motors run in reverse direction while the PWM duty cycle smoothly
 * accelerates from 0 to 255 and then decelerates back to 0.
 */
void speedControl() {
  /** Set motors to reverse direction */
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);

  /** Gradually accelerate from 0 to maximum speed */
  for (int i = 0; i < 256; i++) {
    ledcWrite(enA, i);
    ledcWrite(enB, i);
    delay(20);
  }

  /** Gradually decelerate from maximum to zero */
  for (int i = 255; i >= 0; --i) {
    ledcWrite(enA, i);
    ledcWrite(enB, i);
    delay(20);
  }

  /** Ensure motors are fully stopped */
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}