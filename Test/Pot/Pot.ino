const int potPin1 = 34;

const int potPin2 = 35; 

void setup() {
  Serial.begin(115200);
}

void loop() {
  int valorPot1 = analogRead(potPin1); 

  int valorPot2 = analogRead(potPin2); 

  Serial.print("Pot 1 (GPIO 34): ");
  Serial.print(valorPot1);
  
  Serial.print(" | Pot 2 (GPIO 35): ");
  Serial.println(valorPot2);
  
  delay(100); 
}