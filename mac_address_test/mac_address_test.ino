/**
 * @file mac_address_test.ino
 * @brief Utility to retrieve and display the ESP32 MAC address.
 *
 * This utility sketch prints the device's unique MAC address to the
 * serial monitor. MAC addresses are needed for configuring ESP-NOW
 * peers in the rover communication system.
 *
 * @author Aaryan Pawar, Asaf Iron-Jobes
 * @date December 2024
 * @course CSE 474 - Embedded Systems
 */

#include <WiFi.h>
#include <Arduino.h>

/**
 * @brief Arduino setup function.
 *
 * Initializes serial communication and configures Wi-Fi in station mode
 * to retrieve the MAC address.
 */
void setup() {
  /** Initialize serial communication at 115200 baud */
  Serial.begin(115200);

  /** Set Wi-Fi mode to station for MAC address access */
  WiFi.mode(WIFI_MODE_STA);
  delay(200);

  /** Print the device MAC address to serial */
  Serial.println(WiFi.macAddress());
}

/**
 * @brief Main loop continuously outputs MAC address.
 */
void loop() {
  Serial.println(WiFi.macAddress());
}
