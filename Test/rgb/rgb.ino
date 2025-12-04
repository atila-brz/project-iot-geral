const int buttonPin = 18;

void setup() {
  Serial.begin(115200);
  
  pinMode(buttonPin, INPUT_PULLUP); 
  
  Serial.println("Programa do Botão Inicializado. Pressione o botão...");
}

void loop() {
  int buttonState = digitalRead(buttonPin);
  
  if (buttonState == LOW) {
    Serial.println("Botão Pressionado!");
    
    delay(100); 
  } else {
		Serial.println("Não está Pressionado!");

		delay(100);   
  }
}