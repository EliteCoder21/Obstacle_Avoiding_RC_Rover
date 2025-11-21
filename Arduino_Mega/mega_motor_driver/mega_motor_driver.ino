#include <Wire.h>
#include <Adafruit_MotorShield.h>

// Create the motor shield object
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 

// Motors on M1 and M2
Adafruit_DCMotor *leftMotor  = AFMS.getMotor(1); // M1
Adafruit_DCMotor *rightMotor = AFMS.getMotor(2); // M2

// Default speed (0–255)
uint8_t defaultSpeed = 200;

void setup() {
  Serial.begin(9600);      // Debug
  Serial1.begin(9600);     // ESP32 commands RX1 = Pin 19

  stopMotors();
  Serial.println("Mega ready to receive commands from ESP32.");
}

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

void driveForward(uint8_t speed) {
  leftMotor->setSpeed(speed);
  rightMotor->setSpeed(speed);
  leftMotor->run(FORWARD);
  rightMotor->run(FORWARD);
}

void driveBackward(uint8_t speed) {
  leftMotor->setSpeed(speed);
  rightMotor->setSpeed(speed);
  leftMotor->run(BACKWARD);
  rightMotor->run(BACKWARD);
}

void turnLeft(uint8_t speed) {
  leftMotor->setSpeed(speed);
  rightMotor->setSpeed(speed);
  leftMotor->run(BACKWARD);
  rightMotor->run(FORWARD);
}

void turnRight(uint8_t speed) {
  leftMotor->setSpeed(speed);
  rightMotor->setSpeed(speed);
  leftMotor->run(FORWARD);
  rightMotor->run(BACKWARD);
}

void stopMotors() {
  leftMotor->run(RELEASE);
  rightMotor->run(RELEASE);
}
