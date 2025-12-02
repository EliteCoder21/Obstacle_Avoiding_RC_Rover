#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "soc/timer_group_reg.h"

// Button pin definitions
#define BTN_LEFT_PIN    2   
#define BTN_RIGHT_PIN   3    
#define BTN_BACK_PIN    4    
#define SEND_COMMAND_PERIOD 50000 // in timer ticks (50,000 ticks = 50 ms at 1 MHz)
#define TIMER_INCREMENT_MODE (1 << 30)
#define TIMER_ENABLE (1 << 31)
#define CLOCK_DIVIDER (80 << 13) //80 MHz / 80 = 1 MHz timer clock





// Command enum & payload
typedef enum : uint8_t {
  CMD_STOP = 0,
  CMD_FORWARD,
  CMD_BACKWARD,
  CMD_LEFT,
  CMD_RIGHT
} CarCommand;

// Packet sent over ESP-NOW
typedef struct __attribute__((packed)) {
  uint8_t cmd;   // CarCommand encoded as uint8_t
} ControlPacket;

// =========================
// Globals
// =========================

uint8_t carPeerMac[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }; // TODO: replace with real MAC

CarCommand currentCmd = CMD_STOP;

const unsigned long SEND_PERIOD_MS = 50;  // send command at 20 Hz
unsigned long lastSendMs = 0;

// =========================
// ESP-NOW callback
// =========================
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("ESP-NOW send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");
}

// =========================
// Button helper
// =========================

// Read raw button (with INPUT_PULLUP:
//    pressed  -> LOW (0)
//    released -> HIGH (1)
bool isButtonPressed(uint8_t pin) {
  return digitalRead(pin) == LOW;
}

// Map buttons to a CarCommand
CarCommand computeCommandFromButtons() {
  bool leftPressed  = isButtonPressed(BTN_LEFT_PIN);
  bool rightPressed = isButtonPressed(BTN_RIGHT_PIN);
  bool backPressed  = isButtonPressed(BTN_BACK_PIN);

  // Priority logic:
  //  1) Back button overrides everything
  //  2) Left + Right together = forward
  //  3) Left only or Right only for turning
  //  4) No buttons = stop
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

// =========================
// ESP-NOW setup
// =========================
void setupEspNow() {
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register send callback (optional but nice for debugging)
  esp_now_register_send_cb(onDataSent);

  // Register peer (the car ESP32)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, carPeerMac, 6);
  peerInfo.channel = 0;   // 0 = current WiFi channel
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add ESP-NOW peer");
  } else {
    Serial.println("ESP-NOW peer added");
  }
}

// =========================
// Send function
// =========================
void sendCurrentCommand() {
  ControlPacket pkt;
  pkt.cmd = static_cast<uint8_t>(currentCmd);

  esp_err_t result = esp_now_send(carPeerMac, (uint8_t*)&pkt, sizeof(pkt));
  if (result != ESP_OK) {
    Serial.print("Error sending ESP-NOW packet: ");
    Serial.println(result);
  }
}

// =========================
// Arduino setup / loop
// =========================
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Setup hardware timer for periodic tasks
  uint32_t timer_config |= CLOCK_DIVIDER | TIMER_INCREMENT_MODE | TIMER_ENABLE;
  *((volatile uint32_t *)TIMG_T0CONFIG_REG(0)) = timer_config;

  // Configure button pins (pull-up, active-low)
  pinMode(BTN_LEFT_PIN,  INPUT_PULLUP);
  pinMode(BTN_RIGHT_PIN, INPUT_PULLUP);
  pinMode(BTN_BACK_PIN,  INPUT_PULLUP);

  // ESP32 must be in STA mode for ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();  // not strictly required, but keeps things clean

  setupEspNow();

  Serial.println("Remote controller setup complete.");
}

static uint32_t last_toggle_time = 0;

void loop() {
  
  // 1) Read buttons and compute current command
  currentCmd = computeCommandFromButtons();
  
  // 2) Send at a fixed rate 
  *((volatile uint32_t *)TIMG_T0UPDATE_REG(0)) = 1;
  uint32_t current_time = *((volatile uint32_t *)TIMG_T0LO_REG(0));

  if ((current_time - last_toggle_time) >= SEND_COMMAND_PERIOD) {
    sendCurrentCommand();
    last_toggle_time = current_time;
  }
  
}