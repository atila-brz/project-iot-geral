#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

void setupBluetooth() {
  Serial.println("Iniciando Bluetooth...");
  if (!SerialBT.begin("ESP32_Device")) {
    Serial.println("Ocorreu um erro ao inicializar o Bluetooth");
  } else {
    Serial.println("Bluetooth iniciado com sucesso. Pronto para parear!");
  }
}

void setup() {
  Serial.begin(115200);
  setupBluetooth();
}

void loop() {
  if (SerialBT.available()) {
    char incomingChar = SerialBT.read();
    Serial.print("Recebido via Bluetooth: ");
    Serial.println(incomingChar);
    SerialBT.print("Caractere recebido: ");
    SerialBT.println(incomingChar);
  }
  
  static unsigned long lastStatusCheck = 0;
  if (millis() - lastStatusCheck > 5000) {
    if (SerialBT.hasClient()) {
      Serial.println("Status: Conectado a um dispositivo.");
    } else {
      Serial.println("Status: Aguardando conexão...");
    }
    lastStatusCheck = millis();
  }
  delay(100);
}