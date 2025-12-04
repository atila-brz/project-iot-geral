#include <ESP32Servo.h>

const int servoPin = 26; 

Servo meuServo;

void setup() {
  Serial.begin(115200);
  
  meuServo.attach(servoPin); 
  Serial.println("Servo anexado ao GPIO 25. Iniciando movimento...");
}

void loop() {
  Serial.println("Posição: 0 graus");
  meuServo.write(0);  
  delay(1500);

  Serial.println("Posição: 90 graus");
  meuServo.write(90); 
  delay(1500);

  Serial.println("Posição: 180 graus");
  meuServo.write(180); 
  delay(1500);
}