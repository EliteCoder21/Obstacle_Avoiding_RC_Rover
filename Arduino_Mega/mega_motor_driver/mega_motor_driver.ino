/**
 * @file mega_motor_driver.ino
 * @brief Arduino Mega motor driver for the RC rover.
 *
 * Listens for motion commands sent from the ESP32 over Serial1 and controls
 * two DC motors through the Adafruit Motor Shield (V2). Commands include:
 * FORWARD, BACKWARD, LEFT, RIGHT, and STOP.
 *
 * Motors:
 *  - M1 = leftMotor
 *  - M2 = rightMotor
 *
 * @author Aaryan Pawar, Asaf Iron-Jobes
 * @date December 2024
 * @course CSE 474 - Embedded Systems
 */

#include <Wire.h>
#include <Adafruit_MotorShield.h>

/** Motor shield instance for controlling DC motors */
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 

/** Left motor connected to M1 on the motor shield */
Adafruit_DCMotor *leftMotor  = AFMS.getMotor(1);

/** Right motor connected to M2 on the motor shield */
Adafruit_DCMotor *rightMotor = AFMS.getMotor(2);

/** Default motor speed (0-255 range) */
uint8_t defaultSpeed = 100;

/**
 * @brief Arduino setup routine.
 *
 * Initializes serial ports, starts the motor shield, and ensures motors
 * are stopped before entering the main loop.
 */
void setup() {
  /** Initialize debug serial port at 9600 baud */
  Serial.begin(9600);

  /** Initialize Serial1 for ESP32 communication (RX1 = Pin 19) */
  Serial1.begin(9600);

  /** Initialize the motor shield */
  if (AFMS.begin()) {
    Serial.println("INIT MOTOR DRIVER");
  } else {
    Serial.println("Failed!");
    while (true) {}
  }

  /** Ensure motors are stopped on startup */
  stopMotors();
  Serial.println("Mega ready to receive commands from ESP32.");
}

/**
 * @brief Main Arduino loop.
 *
 * Reads incoming commands from the ESP32 on Serial1 and dispatches the
 * appropriate motor control function.
 */
void loop() {
  /** Check if ESP32 sent a command */
  if (Serial1.available()) {
    String cmd = Serial1.readStringUntil('\n');
    cmd.trim();

    Serial.print("Received: ");
    Serial.println(cmd);

    if (cmd == "FORWARD") {
      driveForward(defaultSpeed);
    } 
    else if (cmd == "BACKWARD") {
      driveBackward(defaultSpeed);
    } 
    else if (cmd == "LEFT") {
      turnLeft(defaultSpeed);
    } 
    else if (cmd == "RIGHT") {
      turnRight(defaultSpeed);
    } 
    else if (cmd == "STOP") {
      stopMotors();
    } 
    else {
      Serial.println("Unknown command.");
    }
  }
}

/**
 * @brief Drives both motors forward at the specified speed.
 * @param speed Motor speed (0-255).
 */
void driveForward(uint8_t speed) {
  leftMotor->setSpeed(speed);
  rightMotor->setSpeed(speed);
  leftMotor->run(FORWARD);
  rightMotor->run(FORWARD);
}

/**
 * @brief Drives both motors backward at the specified speed.
 * @param speed Motor speed (0-255).
 */
void driveBackward(uint8_t speed) {
  leftMotor->setSpeed(speed);
  rightMotor->setSpeed(speed);
  leftMotor->run(BACKWARD);
  rightMotor->run(BACKWARD);
}

/**
 * @brief Turns the rover left by reversing the left wheel and driving the right wheel forward.
 * @param speed Motor speed (0-255).
 */
void turnLeft(uint8_t speed) {
  leftMotor->setSpeed(speed);
  rightMotor->setSpeed(speed);
  leftMotor->run(BACKWARD);
  rightMotor->run(FORWARD);
}

/**
 * @brief Turns the rover right by driving the left wheel forward and reversing the right wheel.
 * @param speed Motor speed (0-255).
 */
void turnRight(uint8_t speed) {
  leftMotor->setSpeed(speed);
  rightMotor->setSpeed(speed);
  leftMotor->run(FORWARD);
  rightMotor->run(BACKWARD);
}

/**
 * @brief Stops both motors by releasing them.
 */
void stopMotors() {
  leftMotor->run(RELEASE);
  rightMotor->run(RELEASE);
}