#include <Arduino.h>
#include "BluetoothSerial.h"
#include "DHT.h"

// --- Definição dos Pinos ---
const int R_PIN = 5;
const int G_PIN = 18;
const int B_PIN = 19;
const int POT_PIN = 26;
const int BUZZER_PIN = 14;
const int DHT_PIN = 4;
const int LDR_PIN = 34;
const int BUTTON_PIN = 22; // --- NOVO --- Pino para o botão de interrupção

// --- Definições do Sensor de Temperatura ---
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

// --- Constantes para cálculo do LDR ---
const float GAMMA = 0.7;
const float RL10 = 50.0;

// --- Objeto Bluetooth Serial ---
BluetoothSerial SerialBT;

// --- Variáveis de Estado ---
bool ledState = false;
bool buzzerActive = false;

// --- NOVO: Variável Mestra de Controle do Sistema ---
// 'volatile' é essencial para variáveis usadas dentro de interrupções
volatile bool systemEnabled = true; 
volatile unsigned long lastInterruptTime = 0; // Para o debounce do botão
// --------------------------------------------------

// --- Variáveis de controle de Temperatura ---
bool temperatureControlActive = false;
float currentTemperature = 20.0;
unsigned long lastTempReadTime = 0;
const long tempReadInterval = 2000;

// --- Variáveis de controle de Luminosidade ---
bool lightDetectorActive = false;
float currentLux = 500.0;
const float LUX_THRESHOLD = 40.0;
unsigned long lastLdrReadTime = 0;
const long ldrReadInterval = 2000;

// --- Variáveis de Controle de Prioridade ---
bool priorityMode = false;
unsigned long priorityStartTime = 0;
const long priorityDuration = 3000;

// --- Variável para Controle do Buzzer ---
int lastColorState = 0;

// --- NOVO: Função de Interrupção (ISR) para o botão ---
// IRAM_ATTR garante que a função seja carregada na RAM para execução rápida
void IRAM_ATTR handleButtonInterrupt() {
  // Debounce: Evita que múltiplos sinais sejam lidos em um único aperto de botão
  if (millis() - lastInterruptTime > 300) {
    systemEnabled = !systemEnabled; // Inverte o estado do sistema (liga/desliga)
    lastInterruptTime = millis();
  }
}
// ----------------------------------------------------

// --- Função para obter média de leituras analógicas ---
int getAverage(int pin, int amostras = 32) {
  long soma = 0;
  for (int i = 0; i < amostras; i++) {
    soma += analogRead(pin);
    delay(1);
  }
  return soma / amostras;
}

// --- Função para tocar o Buzzer (se ativo) ---
void beepOnChange(int newState) {
  if (newState != lastColorState && buzzerActive) {
    tone(BUZZER_PIN, 1000, 50);
  }
  lastColorState = newState;
}

// --- Funções de Controle do LED ---
void setRGBColor(int r_pin, int g_pin, int b_pin, int r_val, int g_val, int b_val) {
  analogWrite(r_pin, r_val);
  analogWrite(g_pin, g_val);
  analogWrite(b_pin, b_val);
}

void updateRGB_with_Potentiometer(int pot_pin, int r_pin, int g_pin, int b_pin) {
  int pot_value = analogRead(pot_pin);
  if (pot_value < 1365) {
    beepOnChange(1); // Vermelho
    setRGBColor(r_pin, g_pin, b_pin, 255, 0, 0);
  } else if (pot_value < 2730) {
    beepOnChange(2); // Amarelo
    setRGBColor(r_pin, g_pin, b_pin, 255, 255, 0);
  } else {
    beepOnChange(3); // Azul
    setRGBColor(r_pin, g_pin, b_pin, 0, 0, 255);
  }
}

// --- Funções de Setup ---
void setupRGB_Pot_Buzzer() {
  pinMode(POT_PIN, INPUT);
  pinMode(R_PIN, OUTPUT);
  pinMode(G_PIN, OUTPUT);
  pinMode(B_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
}

void setupBluetooth() {
  Serial.println("Iniciando Bluetooth...");
  if (!SerialBT.begin("ESP32_LedControl")) {
    Serial.println("Ocorreu um erro ao inicializar o Bluetooth");
  } else {
    Serial.println("Bluetooth iniciado. Pronto para parear!");
  }
}

void setupTemperatureSensor() {
  dht.begin();
  Serial.println("Sensor de temperatura iniciado.");
}

void setupLdr() {
  pinMode(LDR_PIN, INPUT);
  Serial.println("Detector de luminosidade (LDR) iniciado.");
}

void setup() {
  Serial.begin(115200);
  setupBluetooth();
  setupRGB_Pot_Buzzer();
  setupTemperatureSensor();
  setupLdr();
  
  // --- NOVO: Configuração do pino e da interrupção do botão ---
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Usa resistor de pull-up interno
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonInterrupt, FALLING);
  // FALLING: a interrupção é acionada quando o pino vai de HIGH para LOW (botão pressionado)
  // -------------------------------------------------------------

  setRGBColor(R_PIN, G_PIN, B_PIN, 0, 0, 0);
  Serial.println("Setup completo. Sistema ATIVO.");
}

// --- Funções de Leitura dos Sensores ---
void readTemperature() {
  if (millis() - lastTempReadTime >= tempReadInterval) {
    float temp = dht.readTemperature();
    if (!isnan(temp)) {
      currentTemperature = temp;
      Serial.print("Temperatura atual: ");
      Serial.print(currentTemperature);
      Serial.println(" *C");
    }
    lastTempReadTime = millis();
  }
}

void readLuminosity() {
  if (millis() - lastLdrReadTime >= ldrReadInterval) {
    int adcLdr = getAverage(LDR_PIN, 16);
    float voltage = adcLdr * (3.3 / 4095.0);
    float resistance = 10 * (3.3 - voltage) / voltage;
    currentLux = pow(RL10 / resistance, 1 / GAMMA) * 10;
    Serial.print("Luminosidade atual: ");
    Serial.print(currentLux);
    Serial.println(" Lux");
    lastLdrReadTime = millis();
  }
}

void loop() {
  // --- NOVO: A 'chave geral' que controla todo o sistema ---
  if (systemEnabled) {
    // ---- TODO O CÓDIGO OPERACIONAL VAI AQUI DENTRO ----

    // 1. VERIFICA COMANDOS BLUETOOTH
    if (SerialBT.available()) {
      String command = SerialBT.readString();
      command.trim();
      Serial.print("Comando recebido: ");
      Serial.println(command);

      if (command.equalsIgnoreCase("Ligar")) {
        ledState = true; priorityMode = false; SerialBT.println("LED ligado. Controle via potenciometro.");
      } else if (command.equalsIgnoreCase("Desligar")) {
        ledState = false; priorityMode = false; SerialBT.println("LED desligado.");
      } else if (command.equalsIgnoreCase("Vermelho")) {
        ledState = true; priorityMode = true; priorityStartTime = millis(); beepOnChange(1); setRGBColor(R_PIN, G_PIN, B_PIN, 255, 0, 0); SerialBT.println("Prioridade: Vermelho por 3s.");
      } else if (command.equalsIgnoreCase("Amarelo")) {
        ledState = true; priorityMode = true; priorityStartTime = millis(); beepOnChange(2); setRGBColor(R_PIN, G_PIN, B_PIN, 255, 255, 0); SerialBT.println("Prioridade: Amarelo por 3s.");
      } else if (command.equalsIgnoreCase("Azul")) {
        ledState = true; priorityMode = true; priorityStartTime = millis(); beepOnChange(3); setRGBColor(R_PIN, G_PIN, B_PIN, 0, 0, 255); SerialBT.println("Prioridade: Azul por 3s.");
      } else if (command.equalsIgnoreCase("Ativar Buzzer")) {
        buzzerActive = true; SerialBT.println("Buzzer ativado."); tone(BUZZER_PIN, 3000);
      } else if (command.equalsIgnoreCase("Desativar Buzzer")) {
        buzzerActive = false; SerialBT.println("Buzzer desativado."); noTone(BUZZER_PIN);
      } else if (command.equalsIgnoreCase("Ativar Temperatura")) {
        temperatureControlActive = true; SerialBT.println("Controle de temperatura ATIVADO.");
      } else if (command.equalsIgnoreCase("Desativar Temperatura")) {
        temperatureControlActive = false; SerialBT.println("Controle de temperatura DESATIVADO.");
      } else if (command.equalsIgnoreCase("Ativar Detector")) {
        lightDetectorActive = true; SerialBT.println("Detector de luz ATIVADO.");
      } else if (command.equalsIgnoreCase("Desativar Detector")) {
        lightDetectorActive = false; SerialBT.println("Detector de luz DESATIVADO.");
      }
    }

    // 2. LÊ OS SENSORES CONTINUAMENTE
    readTemperature();
    readLuminosity();

    // 3. GERENCIA O MODO DE PRIORIDADE BLUETOOTH
    if (priorityMode && (millis() - priorityStartTime >= priorityDuration)) {
      priorityMode = false;
      Serial.println("Modo de prioridade desativado. Retornando ao potenciometro.");
    }

    // 4. LÓGICA PRINCIPAL DE CONTROLE DO LED (COM HIERARQUIA)
    if (temperatureControlActive && (currentTemperature < 0 || currentTemperature > 26.80)) {
      setRGBColor(R_PIN, G_PIN, B_PIN, 0, 0, 0); beepOnChange(0);
    } else if (lightDetectorActive) {
      if (currentLux > LUX_THRESHOLD) { // Ambiente escuro - CORRIGIDO (era >)
        updateRGB_with_Potentiometer(POT_PIN, R_PIN, G_PIN, B_PIN);
      } else { // Ambiente claro
        setRGBColor(R_PIN, G_PIN, B_PIN, 0, 0, 0); beepOnChange(0);
      }
    } else {
      if (ledState) {
        if (!priorityMode) {
          updateRGB_with_Potentiometer(POT_PIN, R_PIN, G_PIN, B_PIN);
        }
      } else {
        setRGBColor(R_PIN, G_PIN, B_PIN, 0, 0, 0); beepOnChange(0);
      }
    }

  } else {
    // ---- O SISTEMA ESTÁ DESLIGADO ----
    // Garante que todos os atuadores estejam desligados
    setRGBColor(R_PIN, G_PIN, B_PIN, 0, 0, 0);
    noTone(BUZZER_PIN);
    // Podemos opcionalmente resetar os estados aqui se quisermos
    ledState = false;
    buzzerActive = false;
    priorityMode = false;
  }
  
  delay(50); // Delay para estabilidade geral
}