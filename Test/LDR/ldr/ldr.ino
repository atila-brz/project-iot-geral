const int ldrPin = 32; 

const int LIMITE_LUZ = 2000; 

void setup() {
  Serial.begin(115200);
  
  Serial.println("--- Teste do Sensor LDR ---");
  Serial.println("Leitura de luz: 0 (Escuro) a 4095 (Claro)");
  Serial.println("Limite de detecção configurado em: " + String(LIMITE_LUZ));
}

void loop() {
  int ldrValue = analogRead(ldrPin);
  
  Serial.print("Leitura ADC: ");
  Serial.print(ldrValue);
  
  if (ldrValue > LIMITE_LUZ) {
    Serial.println(" -> AMBIENTE CLARO");
  } else {
    Serial.println(" -> AMBIENTE ESCURO");
  }
  
  delay(500); 
}