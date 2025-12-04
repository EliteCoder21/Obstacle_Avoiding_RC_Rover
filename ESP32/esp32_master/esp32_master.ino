/**
 * @file esp32_master.ino
 * @brief Master ESP32-S3 node for an obstacle-avoiding RC rover.
 *
 * Receives high-level drive commands over ESP-NOW from a remote controller,
 * measures obstacle distance with an ultrasonic sensor, and forwards safe
 * motion commands to an Arduino Mega motor driver over serial. Basic safety
 * logic overrides user commands when obstacles are detected.
 */

// Filename: esp32_master/esp32_master.ino
// Author(s): Aaryan Pawar, Asaf Iron-Jobes
// Date: 12/03/2025
// Description: Esp32 Master code for obstacle avoiding RC rover. 
//Uses the esp32s3 and connects to the arduino mega motor driver via serial communication.
// It also uses an ultrasonic sensor to measure distance to obstacles and makes driving decisions accordingly.
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
#define TURN_TIME 100

/**
 * @enum CarCommand
 * @brief High-level motion commands for the rover.
 *
 * These commands are received from the remote controller over ESP-NOW and
 * translated into serial strings for the Arduino Mega motor driver.
 */
typedef enum : uint8_t {
  CMD_STOP = 0,
  CMD_FORWARD,
  CMD_BACKWARD,
  CMD_LEFT,
  CMD_RIGHT
} CarCommand;

// Latest command received from the remote controller.
CarCommand remoteCommand = CMD_STOP;

/**
 * @struct ControlPacket
 * @brief ESP-NOW control packet carrying a single command.
 *
 * Packed to avoid any padding so that the sender and receiver agree on size.
 */
typedef struct __attribute__((packed)) {
  uint8_t cmd;   ///< Encoded CarCommand value.
} ControlPacket;

// Set up serial connection to Arduino Mega (motor slave)
HardwareSerial SerialMega(2); 

// Create the Sonar object
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

// Setup Distance Variable
int measuredDistanceCm = 50;

                    
// Setup Semaphores for safety tracking
SemaphoreHandle_t distanceMutex  = NULL;
SemaphoreHandle_t commandMutex = NULL;

// Setup Task handles for tasks
TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t decisionTaskHandle = NULL;

/**
 * @brief Read distance from the ultrasonic sensor in centimeters.
 *
 * Uses the NewPing library to trigger a measurement. If no echo is detected
 * (raw distance is 0), this is treated as MAX_DISTANCE so that logic can
 * assume a large distance instead of a failed reading.
 *
 * @return Distance to the nearest obstacle in centimeters, with 0 mapped to MAX_DISTANCE.
 */
unsigned int readUltrasonicCM() {
  unsigned int distance = sonar.ping_cm(); 
  
  if(distance == 0) {
    distance = MAX_DISTANCE;
  }

  Serial.println(distance);

  return distance;
}

/**
 * @brief ESP-NOW receive callback for processing incoming remote commands.
 *
 * Validates the packet size, extracts the ControlPacket payload, and updates
 * the shared remoteCommand under a mutex. Also prints a human-readable log
 * of the received command over the debug serial port.
 *
 * @param recv_info Metadata about the received ESP-NOW packet (unused here).
 * @param incomingData Pointer to the raw packet data.
 * @param len Length of the incoming packet in bytes.
 */
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

/**
 * @brief FreeRTOS task that decides safe motion based on distance and remote command.
 *
 * This task periodically reads the latest distance measurement and remote
 * command (both protected by mutexes). When an obstacle is too close, it
 * overrides the user command and backs up; otherwise, it forwards the latest
 * user command to the motor controller.
 *
 * @param parameter Unused pointer required by the FreeRTOS task signature.
 */
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

/**
 * @brief FreeRTOS task that periodically measures distance and updates the shared state.
 *
 * Continuously reads the ultrasonic sensor, writes the latest distance into
 * the shared measuredDistanceCm variable under a mutex, and toggles the alarm
 * output when an obstacle is closer than CRITICAL_THRESHOLD.
 *
 * @param pvParameters Unused pointer required by the FreeRTOS task signature.
 */
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

/**
 * @brief Arduino setup function that initializes hardware, ESP-NOW, and tasks.
 *
 * Configures serial ports, ultrasonic and alarm pins, creates mutexes, sets
 * up Wi-Fi in station mode, initializes ESP-NOW with a receive callback, and
 * starts the decision and sensor tasks pinned to different cores.
 */
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

/**
 * @brief Main Arduino loop left intentionally empty.
 *
 * All logic is handled in FreeRTOS tasks started in setup().
 */
void loop() {}