#include <Arduino.h>
#include <Wire.h>
#include <semphr.h>
#include <WiFi.h>
#include <esp_now.h>  
#include <NewPing.h> 

// Define Pin constants
#define TRIG_PIN 5
#define ECHO_PIN 18
#define ALARM_PIN 4

// Define distance thresholds in centimeters
#define CRITICAL_THRESHOLD 8
#define OBSTACLE_THRESHOLD 20
#define MAX_DISTANCE 100

// Define turn amount
#define TURN_TIME 200

// Command enum & payload 
typedef enum : uint8_t {
  CMD_STOP = 0,
  CMD_FORWARD,
  CMD_BACKWARD,
  CMD_LEFT,
  CMD_RIGHT
} CarCommand;

CarCommand remoteCommand = CMD_STOP;


typedef struct __attribute__((packed)) {
  uint8_t cmd;
} ControlPacket;

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
SemaphoreHandle_t commandMutex = NULL;

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


void receiveCommand(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  // Validate length
  if (len != sizeof(ControlPacket)) {
    Serial.print("Error: Invalid packet size. Expected ");
    Serial.print(sizeof(ControlPacket));
    Serial.print(" bytes, got ");
    Serial.println(len);
    return;
  }

  // Cast incoming data to ControlPacket struct and extract command
  const ControlPacket *pkt = (const ControlPacket *)incomingData;
  CarCommand cmd = (CarCommand)pkt->cmd;

  // Store command safely with mutex
  if (xSemaphoreTake(commandMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    remoteCommand = cmd;
    xSemaphoreGive(commandMutex);
  }

  // debug only prints
  switch (cmd) {
    case CMD_STOP:    Serial.println("Command: STOP");    break;
    case CMD_FORWARD: Serial.println("Command: FORWARD"); break;
    case CMD_BACKWARD:Serial.println("Command: BACKWARD");break;
    case CMD_LEFT:    Serial.println("Command: LEFT");    break;
    case CMD_RIGHT:   Serial.println("Command: RIGHT");   break;
    default:          Serial.println("Command: UNKNOWN");  break;
  }
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

    // Get the remote command
    CarCommand cmd = CMD_STOP;
    if (xSemaphoreTake(commandMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      cmd = remoteCommand;
      xSemaphoreGive(commandMutex);
    }

    // Make the Decision
    if (currentDist <= CRITICAL_THRESHOLD) {

      // Debugging
      Serial.println("Proximity Alert! Backtracking...");

      // Go back for a short time
      SerialMega.println("BACKWARD");   
      vTaskDelay(pdMS_TO_TICKS(TURN_TIME)); 

    } else if (currentDist <= OBSTACLE_THRESHOLD) {

      // Debugging
      Serial.println("Obstacle Alert! Backtracking...");

      // Back up for a short time, then return control to user
      SerialMega.println("BACKWARD");   
      vTaskDelay(pdMS_TO_TICKS(TURN_TIME)); 

    } else {

      // No obstacle, follow remote command
      switch (cmd) {
        case CMD_STOP:
          SerialMega.println("STOP");
          break;
        case CMD_FORWARD:
          SerialMega.println("FORWARD");
          break;
        case CMD_BACKWARD:
          SerialMega.println("BACKWARD");
          break;
        case CMD_LEFT:
          SerialMega.println("LEFT");
          break;
        case CMD_RIGHT:
          SerialMega.println("RIGHT");
          break;
      }
      vTaskDelay(pdMS_TO_TICKS(50)); 
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
  commandMutex = xSemaphoreCreateMutex();

  // Setup WiFi and ESP-NOW
  Serial.println("Initializing WiFi and ESP-NOW...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  } else {
    Serial.println("ESP-NOW initialized");
    esp_now_register_recv_cb(receiveCommand);
  }


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
