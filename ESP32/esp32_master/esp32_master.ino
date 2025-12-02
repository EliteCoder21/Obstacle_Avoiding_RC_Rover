#include <Arduino.h>
#include <Wire.h>
#include <NewPing.h>
#include <semphr.h>

// Define Pin constants
#define TRIG_PIN 5
#define ECHO_PIN 18
#define ALARM_PIN 4

// Define distance thresholds in centimeters
#define CRITICAL_THRESHOLD 10
#define OBSTACLE_THRESHOLD 30
#define MAX_DISTANCE 100

// Define turn amount
#define TURN_TIME 200

// Set up serial connection to Arduino Mega (motor slave)
HardwareSerial SerialMega(2); 

// Create the Sonar object
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

// Setup Distance Variable
int measuredDistanceCm = 50;
int leftMeasuredDistanceCm = 50;
int rightMeasuredDistanceCm = 50;

// Setup best direction variable
int bestDirection = 1; // 0 is for left
                       // 1 is for forward
                       // 2 is for right                       

// Setup Semaphores for safety tracking
SemaphoreHandle_t distanceMutex  = NULL;
SemaphoreHandle_t directionMutex = NULL;

// Setup Task handles for tasks
TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t decisionTaskHandle = NULL;

// Function to get distance value
unsigned int readUltrasonicCM() {
  unsigned int distance = sonar.ping_cm(); 
  
  if(distance == 0) {
    distance = MAX_DISTANCE;
  }

  Serial.println(distance);

  return distance;
}

// Driving Decision Task
void decisionTask(void *parameter) {

  while (1) {

    // Get the distance measured by the ultrasonic sensor
    int currentDist;
    if (xSemaphoreTake(distanceMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      currentDist = measuredDistanceCm;
      xSemaphoreGive(distanceMutex);
    } else {
      currentDist = MAX_DISTANCE;
    }

    // Make the Decision
    if (currentDist <= CRITICAL_THRESHOLD) {

      // Debugging
      Serial.println("Proximity Alert! Backtracking...");

      // Go back if we see something really close
      SerialMega.println("BACKWARD");   
      vTaskDelay(pdMS_TO_TICKS(500)); 

    } else if (currentDist <= OBSTACLE_THRESHOLD) {

      // Debugging
      Serial.println("Obstacle Alert! Scanning for optimal route...");

      // Scan around to find a safe path

      // Scan left
      SerialMega.println("LEFT");
      vTaskDelay(pdMS_TO_TICKS(TURN_TIME));

      // Record distance
      leftMeasuredDistanceCm = readUltrasonicCM();

      // Scan right
      SerialMega.println("RIGHT");
      vTaskDelay(pdMS_TO_TICKS(2 * TURN_TIME));
    
      // Record distance
      rightMeasuredDistanceCm = readUltrasonicCM();    

      // Position the rover appropriately
      if (rightMeasuredDistanceCm < leftMeasuredDistanceCm) {
        SerialMega.println("LEFT");
        vTaskDelay(pdMS_TO_TICKS(2 * TURN_TIME));
      }

    } else {

      // Debugging
      Serial.println("All clear! Proceeding forward...");

      SerialMega.println("FORWARD");   
      vTaskDelay(pdMS_TO_TICKS(500)); 
    }   
  
  }
}

// Sensor task: periodically measures distance and updates shared variable
void sensorTask(void* pvParameters) {
  while (1) {

    // Read the value
    unsigned int d = readUltrasonicCM();

    // Write the value
    if (xSemaphoreTake(distanceMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      measuredDistanceCm = d;
      xSemaphoreGive(distanceMutex);
    }

    // Sound the alarm if necessary
    if (d < CRITICAL_THRESHOLD) {
      digitalWrite(ALARM_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(200)); // 200 ms on
      digitalWrite(ALARM_PIN, LOW);
      vTaskDelay(pdMS_TO_TICKS(200)); // 200 ms off
    }   
    
    // Delay if necessary
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// Setup function
void setup() {

  // Start up Serial
  Serial.begin(9600);
  SerialMega.begin(9600, SERIAL_8N1, 16, 2);

  // Create the tasks
  Serial.println("Initializing...");

  // Setup ultrasonic sensor and beeper pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(ALARM_PIN, OUTPUT);

  // Create Mutex and give to Decision Task
  distanceMutex = xSemaphoreCreateMutex();

  // Create the tasks
  Serial.println("Creating Tasks...");

  // Create Tasks
  xTaskCreatePinnedToCore(
    decisionTask,
    "Decision",
    4096,
    NULL,
    1,
    &decisionTaskHandle,
    0                       // core 0
  );
  
  xTaskCreatePinnedToCore(
    sensorTask,
    "Sensor",
    4096,
    NULL,
    1,
    &sensorTaskHandle,
    1                       // core 1
  );
}

void loop() {}
