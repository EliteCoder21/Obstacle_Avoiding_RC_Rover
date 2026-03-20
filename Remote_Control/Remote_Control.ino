/**
 * @file Remote_Control.ino
 * @brief ESP32-based remote controller for the obstacle-avoiding RC rover.
 *
 * Reads button inputs to determine a desired CarCommand and sends it
 * periodically to the rover over ESP-NOW. A hardware timer is used to
 * pace command transmissions at a fixed rate.
 *
 * Button Layout:
 *  - Left button:  Turn left
 *  - Right button: Turn right
 *  - Both buttons: Drive forward
 *  - Back button:  Reverse (highest priority)
 *  - No buttons:   Stop
 *
 * @author Aaryan Pawar, Asaf Iron-Jobes
 * @date December 2024
 * @course CSE 474 - Embedded Systems
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "soc/timer_group_reg.h"

/** GPIO pin for left direction button (active-low with pull-up) */
#define BTN_LEFT_PIN    4

/** GPIO pin for right direction button (active-low with pull-up) */
#define BTN_RIGHT_PIN   5

/** GPIO pin for reverse/back button (active-low with pull-up) */
#define BTN_BACK_PIN    6

/** Periodic send interval in hardware timer ticks (40ms at 1MHz) */
#define SEND_COMMAND_PERIOD 40000

/** Timer configuration: increment mode */
#define TIMER_INCREMENT_MODE (1 << 30)

/** Timer configuration: enable timer */
#define TIMER_ENABLE (1 << 31)

/** Clock divider: 80 MHz / 80 = 1 MHz timer clock */
#define CLOCK_DIVIDER (80 << 13)

/**
 * @enum CarCommand
 * @brief High-level motion commands for the rover.
 *
 * These symbolic commands are mapped from button presses on the controller
 * and transmitted over ESP-NOW to the rover.
 */
typedef enum : uint8_t {
  CMD_STOP = 0,
  CMD_FORWARD,
  CMD_BACKWARD,
  CMD_LEFT,
  CMD_RIGHT
} CarCommand;

/**
 * @struct ControlPacket
 * @brief Packet structure sent over ESP-NOW.
 *
 * Contains a single encoded CarCommand. The struct is packed to ensure
 * sender and receiver agree on the payload size.
 */
typedef struct __attribute__((packed)) {
  uint8_t cmd;
} ControlPacket;

/**
 * @brief MAC address of the rover's ESP32 receiver.
 *
 * Peer address for ESP-NOW communication with the rover.
 */
uint8_t carPeerMac[] = { 0xB8, 0xF8, 0x62, 0xE0, 0x84, 0x2C }; 

/** Latest command computed from button state */
CarCommand currentCmd = CMD_STOP;

/**
 * @brief ESP-NOW send callback for reporting transmission status.
 *
 * Logs whether the last ESP-NOW send operation completed successfully.
 *
 * @param info   Pointer to tx info metadata (unused here).
 * @param status Status of the send operation.
 */
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("ESP-NOW send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");
}

/**
 * @brief Check if a button connected to the given pin is pressed.
 *
 * Assumes the pin is configured as INPUT_PULLUP so that:
 *  - Pressed  -> LOW (0)
 *  - Released -> HIGH (1)
 *
 * @param pin GPIO pin number where the button is connected.
 * @return true if the button is currently pressed, false otherwise.
 */
bool isButtonPressed(uint8_t pin) {
  return digitalRead(pin) == LOW;
}

/**
 * @brief Map current button states to a CarCommand.
 *
 * Priority rules:
 *  1. Back button overrides everything (reverse).
 *  2. Left + Right together = forward.
 *  3. Left only or Right only are turning commands.
 *  4. No buttons pressed -> stop.
 *
 * @return CarCommand that best represents the current button combination.
 */
CarCommand computeCommandFromButtons() {
  bool leftPressed  = isButtonPressed(BTN_LEFT_PIN);
  bool rightPressed = isButtonPressed(BTN_RIGHT_PIN);
  bool backPressed  = isButtonPressed(BTN_BACK_PIN);

  if (backPressed) {
    return CMD_BACKWARD;
  } else if (leftPressed && rightPressed) {
    return CMD_FORWARD;
  } else if (leftPressed) {
    return CMD_LEFT;
  } else if (rightPressed) {
    return CMD_RIGHT;
  } else {
    return CMD_STOP;
  }
}

/**
 * @brief Initialize ESP-NOW and register the rover as a peer.
 *
 * Sets up ESP-NOW, registers a send callback for debugging, and adds
 * the carPeerMac as a peer. Retries adding the peer until success.
 */
void setupEspNow() {
  /** Initialize ESP-NOW */
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  /** Register send callback for debugging */
  esp_now_register_send_cb(onDataSent);

  /** Register the car ESP32 as a peer */
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, carPeerMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  while (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add ESP-NOW peer");
    delay(100);
  }
  Serial.println("ESP-NOW peer added");
}

/**
 * @brief Send the current command to the rover over ESP-NOW.
 *
 * Wraps the global currentCmd into a ControlPacket and attempts to send
 * it to the configured carPeerMac. Errors are printed over serial.
 */
void sendCurrentCommand() {
  ControlPacket pkt;
  pkt.cmd = static_cast<uint8_t>(currentCmd);

  esp_err_t result = esp_now_send(carPeerMac, (uint8_t*)&pkt, sizeof(pkt));
  if (result != ESP_OK) {
    Serial.print("Error sending ESP-NOW packet: ");
    Serial.println(result);
  }
}

/**
 * @brief Arduino setup function.
 *
 * Initializes serial output, configures the hardware timer for periodic
 * tasks, sets up button GPIOs with pull-ups, configures Wi-Fi in station
 * mode, and initializes ESP-NOW and its peer.
 */
void setup() {
  Serial.begin(9600);
  delay(1000);

  /** Configure hardware timer for periodic command transmission */
  uint32_t timer_config = CLOCK_DIVIDER | TIMER_INCREMENT_MODE | TIMER_ENABLE;
  *((volatile uint32_t *)TIMG_T0CONFIG_REG(0)) = timer_config;

  /** Configure button pins as inputs with pull-up resistors (active-low) */
  pinMode(BTN_LEFT_PIN,  INPUT_PULLUP);
  pinMode(BTN_RIGHT_PIN, INPUT_PULLUP);
  pinMode(BTN_BACK_PIN,  INPUT_PULLUP);

  /** Configure Wi-Fi in station mode for ESP-NOW */
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  setupEspNow();

  Serial.println("Remote controller setup complete.");
}

/** Timestamp of last command send based on hardware timer ticks */
static uint32_t last_toggle_time = 0;

/**
 * @brief Main Arduino loop.
 *
 * Continuously reads the buttons to compute the current command and sends
 * it to the rover at a fixed rate using the hardware timer as a timebase.
 */
void loop() {
  /** Read buttons and compute current command */
  currentCmd = computeCommandFromButtons();

  /** Send at a fixed rate using hardware timer */
  *((volatile uint32_t *)TIMG_T0UPDATE_REG(0)) = 1;
  uint32_t current_time = *((volatile uint32_t *)TIMG_T0LO_REG(0));

  if ((current_time - last_toggle_time) >= SEND_COMMAND_PERIOD) {
    sendCurrentCommand();
    last_toggle_time = current_time;
    Serial.println(currentCmd);
  }
}