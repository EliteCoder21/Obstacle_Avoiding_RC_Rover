/**
 * @file esp32_master.ino
 * @brief Master ESP32-S3 node for an obstacle-avoiding RC rover.
 *
 * Receives high-level drive commands over ESP-NOW from a remote controller,
 * measures obstacle distance with an ultrasonic sensor, and forwards safe
 * motion commands to an Arduino Mega motor driver over serial. Basic safety
 * logic overrides user commands when obstacles are detected.
 *
 * @author Aaryan Pawar, Asaf Iron-Jobes
 * @date December 2024
 * @course CSE 474 - Embedded Systems
 */

#include <Arduino.h>
#include <Wire.h>
#include <semphr.h>
#include <WiFi.h>
#include <esp_now.h>  
#include <NewPing.h> 

/** Ultrasonic sensor trigger pin */
#define TRIG_PIN 5

/** Ultrasonic sensor echo pin */
#define ECHO_PIN 18

/** Alarm/buzzer output pin */
#define ALARM_PIN 4

/** Minimum distance to obstacle that triggers immediate stop (cm) */
#define CRITICAL_THRESHOLD 8

/** Distance to obstacle that triggers warning and cautious movement (cm) */
#define OBSTACLE_THRESHOLD 20

/** Maximum measurable distance for the ultrasonic sensor (cm) */
#define MAX_DISTANCE 100

/** Duration to back up when avoiding obstacles (ms) */
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

/** Latest command received from the remote controller */
CarCommand remoteCommand = CMD_STOP;

/**
 * @struct ControlPacket
 * @brief ESP-NOW control packet carrying a single command.
 *
 * Packed to avoid any padding so that the sender and receiver agree on size.
 */
typedef struct __attribute__((packed)) {
  uint8_t cmd;
} ControlPacket;

/** Serial connection to Arduino Mega (UART2: RX=16, TX=2) */
HardwareSerial SerialMega(2); 

/** Ultrasonic sensor object for distance measurement */
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

/** Current distance measured by ultrasonic sensor (cm) */
int measuredDistanceCm = 50;

/** Mutex for thread-safe access to distance variable */
SemaphoreHandle_t distanceMutex  = NULL;

/** Mutex for thread-safe access to command variable */
SemaphoreHandle_t commandMutex = NULL;

/** Task handle for the sensor reading task */
TaskHandle_t sensorTaskHandle = NULL;

/** Task handle for the decision-making task */
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

<<<<<<< HEAD

=======
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
  /** Validate packet length matches expected ControlPacket size */
  if (len != sizeof(ControlPacket)) {
    Serial.print("Error: Invalid packet size. Expected ");
    Serial.print(sizeof(ControlPacket));
    Serial.print(" bytes, got ");
    Serial.println(len);
    return;
  }

  /** Cast incoming data to ControlPacket and extract command */
  const ControlPacket *pkt = (const ControlPacket *)incomingData;
  CarCommand cmd = (CarCommand)pkt->cmd;

  /** Store command safely with mutex protection */
  if (xSemaphoreTake(commandMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    remoteCommand = cmd;
    xSemaphoreGive(commandMutex);
  }

  /** Debug: print human-readable command */
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

    /** Get the distance measured by the ultrasonic sensor */
    int currentDist;
    if (xSemaphoreTake(distanceMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      currentDist = measuredDistanceCm;
      xSemaphoreGive(distanceMutex);
    } else {
      currentDist = MAX_DISTANCE;
    }

    /** Get the remote command */
    CarCommand cmd = CMD_STOP;
    if (xSemaphoreTake(commandMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      cmd = remoteCommand;
      xSemaphoreGive(commandMutex);
    }

    /** Safety logic: determine appropriate action based on distance */
    if (currentDist <= CRITICAL_THRESHOLD) {
      Serial.println("Proximity Alert! Backtracking...");
      SerialMega.println("BACKWARD");
      vTaskDelay(pdMS_TO_TICKS(TURN_TIME));

    } else if (currentDist <= OBSTACLE_THRESHOLD) {
      Serial.println("Obstacle Alert! Backtracking...");
      SerialMega.println("BACKWARD");
      vTaskDelay(pdMS_TO_TICKS(TURN_TIME));

    } else {
      /** No obstacle detected - follow remote command */
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

<<<<<<< HEAD

// Driving Decision Task
=======
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
>>>>>>> 5852e8ffe2c0882258f49bd39ec8ff2669c4e93c
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
    /** Read distance from ultrasonic sensor */
    unsigned int d = readUltrasonicCM();

    /** Update shared distance variable with mutex protection */
    if (xSemaphoreTake(distanceMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      measuredDistanceCm = d;
      xSemaphoreGive(distanceMutex);
    }

    /** Sound alarm when obstacle is critically close */
    if (d < CRITICAL_THRESHOLD) {
      digitalWrite(ALARM_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(200));
      digitalWrite(ALARM_PIN, LOW);
      vTaskDelay(pdMS_TO_TICKS(200));
    }

    /** Task loop delay */
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
  /** Initialize debug serial port */
  Serial.begin(9600);

  /** Initialize serial connection to Arduino Mega (UART2) */
  SerialMega.begin(9600, SERIAL_8N1, 16, 2);

  Serial.println("Initializing...");

  /** Configure GPIO pins for ultrasonic sensor and alarm */
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(ALARM_PIN, OUTPUT);

  /** Create mutexes for thread-safe variable access */
  distanceMutex = xSemaphoreCreateMutex();
  commandMutex = xSemaphoreCreateMutex();

  /** Initialize Wi-Fi in station mode for ESP-NOW */
  Serial.println("Initializing WiFi and ESP-NOW...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  /** Initialize ESP-NOW and register receive callback */
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  } else {
    Serial.println("ESP-NOW initialized");
    esp_now_register_recv_cb(receiveCommand);
  }

  Serial.println("Creating Tasks...");

  /** Create decision task on core 0 */
  xTaskCreatePinnedToCore(
    decisionTask,
    "Decision",
    4096,
    NULL,
    1,
    &decisionTaskHandle,
    0
  );

  /** Create sensor task on core 1 */
  xTaskCreatePinnedToCore(
    sensorTask,
    "Sensor",
    4096,
    NULL,
    1,
    &sensorTaskHandle,
    1
  );
}

/**
 * @brief Main Arduino loop left intentionally empty.
 *
 * All logic is handled in FreeRTOS tasks started in setup().
 */
void loop() {}