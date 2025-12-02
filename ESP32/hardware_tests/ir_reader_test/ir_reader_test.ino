// ESP32-S3 Analog IR sensor reader

#define IR_SENSOR_PIN 16

void setup() {
  Serial.begin(9600);
  delay(500);

  Serial.println("IR Sensor Reading on GPIO16");
}

void loop() {
  // Read raw ADC value (0–4095 on ESP32-S3)
  int irValue = analogRead(IR_SENSOR_PIN);

  Serial.print("IR Value: ");
  Serial.println(irValue);

  delay(100);
}