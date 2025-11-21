HardwareSerial SerialMega(2); // UART2, GPIO16=RX, GPIO17=TX (adjust for your board)

void setup() {
  Serial.begin(9600);
  SerialMega.begin(9600, SERIAL_8N1, 16, 2); // RX, TX
}

void loop() {
  SerialMega.println("FORWARD");
  delay(2000);

  SerialMega.println("LEFT");
  delay(1000);

  SerialMega.println("RIGHT");
  delay(1000);

  SerialMega.println("BACKWARD");
  delay(2000);

  SerialMega.println("STOP");
  delay(3000);
}
