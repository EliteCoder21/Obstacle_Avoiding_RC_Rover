// ESP32-S3 IR LED driver (38 kHz)
//

#define IR_LED_PIN 3
#define PWM_CHANNEL 0
#define PWM_FREQ 38000    // 38 kHz
#define PWM_RES 8         // 8-bit resolution

void setup() {
  // Set up LEDC PWM
  ledcAttach(IR_LED_PIN, PWM_FREQ, PWM_RES);
     
  // Write the Duty Cycle
  ledcWrite(IR_LED_PIN, 128);
}

void loop() {
  
   
}
