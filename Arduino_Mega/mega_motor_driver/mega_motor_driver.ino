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
 */

#include <Wire.h>
#include <Adafruit_MotorShield.h>

// Create the motor shield object
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 

// Motors on M1 and M2
Adafruit_DCMotor *leftMotor  = AFMS.getMotor(1); // M1
Adafruit_DCMotor *rightMotor = AFMS.getMotor(2); // M2

// Default speed (0–255)
uint8_t defaultSpeed = 100;

/**
 * @brief Arduino setup routine.
 *
 * Initializes serial ports, starts the motor shield, and ensures motors
 * are stopped before entering the main loop.
 */
void setup() {
  Serial.begin(9600);      // Debug
  Serial1.begin(9600);     // ESP32 commands RX1 = Pin 19
<<<<<<< HEAD
  if (AFMS.begin()) {
    Serial.println("INIT MOTOR DRIVER");
  } else {
    Serial.println("Failed!");

    while (true) {}
  }

=======
  
  AFMS.begin();
>>>>>>> 5852e8ffe2c0882258f49bd39ec8ff2669c4e93c
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
  // Check if ESP32 sent a command
  if (Serial1.available()) {
    String cmd = Serial1.readStringUntil('\n');
    cmd.trim();  // Remove whitespace/newlines

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

// ================= Motor Control Functions =================

/**
 * @brief Drives both motors forward at the specified speed.
 * @param speed Motor speed (0–255).
 */
void driveForward(uint8_t speed) {
  leftMotor->setSpeed(speed);
  rightMotor->setSpeed(speed);
  leftMotor->run(FORWARD);
  rightMotor->run(FORWARD);
}

/**
 * @brief Drives both motors backward at the specified speed.
 * @param speed Motor speed (0–255).
 */
void driveBackward(uint8_t speed) {
  leftMotor->setSpeed(speed);
  rightMotor->setSpeed(speed);
  leftMotor->run(BACKWARD);
  rightMotor->run(BACKWARD);
}

/**
 * @brief Turns the rover left by reversing the left wheel and driving the right wheel forward.
 * @param speed Motor speed (0–255).
 */
void turnLeft(uint8_t speed) {
  leftMotor->setSpeed(speed);
  rightMotor->setSpeed(speed);
  leftMotor->run(BACKWARD);
  rightMotor->run(FORWARD);
}

/**
 * @brief Turns the rover right by driving the left wheel forward and reversing the right wheel.
 * @param speed Motor speed (0–255).
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