// --- BIBLIOTECAS ---
#include <Arduino.h>
#include <Bluepad32.h>    // Para o controle Bluetooth
#include <ESP32Servo.h>   // Para os motores
#include "DHT.h"          // Para o sensor de temperatura e umidade
// --- NOVO: Bibliotecas para WiFi e HTTP ---
#include <WiFi.h>
#include <HTTPClient.h>
#include <UrlEncode.h>

// --- NOVO: Configurações de Rede e Callmebot ---
// IMPORTANTE: Substitua com suas informações
const char* ssid = "ZEGAneT";
const char* password = "12345678";
String phoneNumber = "557182829835"; // Formato: 5571... (sem o primeiro 9)
String apiKey = "7301051";     // Sua APIKEY do Callmebot

// --- DEFINIÇÃO DOS PINOS ---
const int SERVO_ESQ_PIN = 33;
const int SERVO_DIR_PIN = 23;
const int DHT_PIN = 4;
const int LDR_PIN = 34;
const int PIR_PIN = 12;
const int LED_R_PIN = 5;
const int LED_G_PIN = 18;
const int LED_B_PIN = 19;
const int BUTTON_PIN = 22;

// --- CONFIGURAÇÕES GERAIS ---
const int ANALOG_DEADZONE = 50;
const int POS_NEUTRA = 90;
const int POS_AVANTE = 135;
const int POS_RE = 45;
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);
const float GAMMA = 0.7;
const float RL10 = 50.0;
const float TEMP_MIN = 15.0;
const float TEMP_MAX = 25.60;
const float UMID_MIN = 40.0;
const float UMID_MAX = 70.0;
const float LUZ_ADEQUADA_THRESHOLD = 50;

// --- OBJETOS GLOBAIS ---
Servo servoEsq;
Servo servoDir;
ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// Variáveis de sensores
float temperatura = 0.0;
float umidade = 0.0;
float intensidadeLuz = 0.0;
bool presencaDetectada = false;
int probabilidadeVida = 0;

// Variáveis de tempo e estado
unsigned long previousMillis = 0;
const long interval = 2000;
volatile bool systemEnabled = true;
volatile unsigned long lastInterruptTime = 0;
bool lastSystemState = true;
// --- NOVO: Flag para controlar o envio da mensagem ---
bool alertaWhatsappEnviado = false;


// --- PROTÓTIPOS DE FUNÇÕES ---
void pararServos();
void setLedStatus(bool alerta);
void lerSensores();
void calcularEProcessarProbabilidade();
void setupWifi(); // --- NOVO
void sendMessage(String message); // --- NOVO

// --- NOVO: Função para conectar ao Wi-Fi ---
void setupWifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

// --- NOVO: Função para enviar mensagem via Callmebot ---
void sendMessage(String message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Erro: Sem conexão Wi-Fi para enviar mensagem.");
    return;
  }
  
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(message);
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpResponseCode = http.POST(url);
  if (httpResponseCode == 200) {
    Serial.println("Mensagem de WhatsApp enviada com sucesso!");
  } else {
    Serial.println("Erro ao enviar mensagem de WhatsApp!");
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

// --- Função de Interrupção (ISR) para o botão ---
void IRAM_ATTR handleButtonInterrupt() {
  if (millis() - lastInterruptTime > 500) {
    systemEnabled = !systemEnabled;
    lastInterruptTime = millis();
  }
}

// --- FUNÇÕES DE CONTROLE DE ATUADORES (MOTORES E LED) ---
void pararServos() {
  servoEsq.write(POS_NEUTRA);
  servoDir.write(POS_NEUTRA);
}

void setLedStatus(bool alerta) {
  if (alerta) {
    analogWrite(LED_R_PIN, 255);
    analogWrite(LED_G_PIN, 0);
    analogWrite(LED_B_PIN, 0);
  } else {
    analogWrite(LED_R_PIN, 0);
    analogWrite(LED_G_PIN, 255);
    analogWrite(LED_B_PIN, 0);
  }
}

// --- FUNÇÕES DE LEITURA DOS SENSORES ---
void lerSensores() {
  // ... (código original sem alterações)
  float temp_lida = dht.readTemperature();
  float umid_lida = dht.readHumidity();
  if (!isnan(temp_lida) && !isnan(umid_lida)) {
    temperatura = temp_lida;
    umidade = umid_lida;
  }
  int adcLdr = analogRead(LDR_PIN);
  float voltage = adcLdr * (3.3 / 4095.0);
  float resistance = 10 * (3.3 - voltage) / voltage;
  intensidadeLuz = pow(RL10 / resistance, 1 / GAMMA) * 10;
  if (isnan(intensidadeLuz)) intensidadeLuz = 0;
  presencaDetectada = digitalRead(PIR_PIN) == HIGH;
}

// --- FUNÇÃO DE "INTELIGÊNCIA" DO ROBÔ ---
void calcularEProcessarProbabilidade() {
  int probabilidade = 0;
  if (temperatura >= TEMP_MIN && temperatura <= TEMP_MAX) probabilidade += 25;
  if (umidade >= UMID_MIN && umidade <= UMID_MAX) probabilidade += 25;
  if (intensidadeLuz < LUZ_ADEQUADA_THRESHOLD) probabilidade += 25;
  if (presencaDetectada) probabilidade += 25;

  probabilidadeVida = min(probabilidade, 100);

  // ... (impressão do relatório no Serial continua igual)
  Serial.println("\n--- [RELATÓRIO DO AMBIENTE] ---");
  Serial.print("Temperatura: "); Serial.print(temperatura); Serial.println(" *C");
  Serial.print("Umidade: "); Serial.print(umidade); Serial.println(" %");
  Serial.print("Luminosidade: "); Serial.print(intensidadeLuz); Serial.println(" Lux");
  Serial.print("Presença: "); Serial.println(presencaDetectada ? "Detectada" : "Nenhuma");
  Serial.println("---------------------------------");
  Serial.print("Probabilidade de vida: "); Serial.print(probabilidadeVida); Serial.println("%");


  if (probabilidadeVida >= 75) {
    setLedStatus(true);
    Serial.println("Status do Robô: ALERTA!");
    Serial.println(">> ALERTA! Alta probabilidade de vida detectada!");

    // --- NOVO: Lógica de envio de mensagem de alerta ---
    if (!alertaWhatsappEnviado) {
      String mensagemDeAlerta = "⚠️ *ALERTA DO ROBÔ EXPLORADOR* ⚠️\n\n";
      mensagemDeAlerta += "Alta probabilidade de vida detectada (" + String(probabilidadeVida) + "%).\n\n";
      mensagemDeAlerta += "*- Dados do Ambiente -*\n";
      mensagemDeAlerta += "Temperatura: " + String(temperatura, 1) + "°C\n";
      mensagemDeAlerta += "Umidade: " + String(umidade, 1) + "%\n";
      mensagemDeAlerta += "Luz: " + String(intensidadeLuz, 0) + " Lux\n";
      mensagemDeAlerta += "Presença: " + String(presencaDetectada ? "SIM" : "NÃO");
      
      sendMessage(mensagemDeAlerta);
      alertaWhatsappEnviado = true; // Marca que a mensagem foi enviada
    }
    
  } else {
    setLedStatus(false);
    Serial.println("Status do Robô: Normal");
    Serial.println(">> Exploração normal. Nenhum indício relevante detectado.");

    // --- NOVO: Reseta a flag quando o sistema volta ao normal ---
    alertaWhatsappEnviado = false;
  }
  Serial.println("---------------------------------");
}

// --- FUNÇÕES DE CONTROLE DO ROBÔ (BLUETOOTH) ---
void processarControleServo(ControllerPtr ctl) {
  int valorEixoY_Esq = ctl->axisY();
  int valorEixoY_Dir = ctl->axisRY();

  if (abs(valorEixoY_Esq) < ANALOG_DEADZONE) valorEixoY_Esq = 0;
  if (abs(valorEixoY_Dir) < ANALOG_DEADZONE) valorEixoY_Dir = 0;

  valorEixoY_Esq = -valorEixoY_Esq;
  valorEixoY_Dir = -valorEixoY_Dir;

  int velocidadeServoEsq = map(valorEixoY_Esq, -512, 512, POS_RE, POS_AVANTE);
  int velocidadeServoDir = map(valorEixoY_Dir, -512, 512, POS_RE, POS_AVANTE);

  servoEsq.write(velocidadeServoEsq);
  servoDir.write(velocidadeServoDir);
}

void onConnectedController(ControllerPtr ctl) {
  bool foundEmptySlot = false;
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      Serial.printf("CONTROLADOR CONECTADO: Armazenado no índice %d\n", i);
      Serial.printf("Modelo: %s\n", ctl->getModelName().c_str());
      myControllers[i] = ctl;
      foundEmptySlot = true;
      break;
    }
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == ctl) {
      Serial.printf("CONTROLADOR DESCONECTADO: Removido do índice %d\n", i);
      myControllers[i] = nullptr;
      pararServos();
      Serial.println("Servos parados por segurança.");
      break;
    }
  }
}

// --------------------------------------------------------------------
// --- SETUP E LOOP ---
// --------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  
  // --- NOVO: Conecta ao Wi-Fi ---
  setupWifi();

  servoEsq.attach(SERVO_ESQ_PIN);
  servoDir.attach(SERVO_DIR_PIN);
  pararServos();
  Serial.println("Servos inicializados.");

  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  dht.begin();
  Serial.println("Pinos de sensores e LED configurados.");
  
  setLedStatus(false);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonInterrupt, FALLING);
  Serial.println("Botão de desligamento geral ATIVADO no pino " + String(BUTTON_PIN));

  Serial.println("Iniciando Bluepad32. Coloque o controle em modo de pareamento...");
  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.forgetBluetoothKeys();

  Serial.println("\nSetup completo. Sistema ATIVO.");
}

void loop() {
  if (systemEnabled != lastSystemState) {
    if (systemEnabled) {
      Serial.println("\n--- SISTEMA REATIVADO PELO BOTÃO ---");
      setLedStatus(false); 
    } else {
      Serial.println("\n--- SISTEMA DESATIVADO PELO BOTÃO ---");
      pararServos();
      analogWrite(LED_R_PIN, 0);
      analogWrite(LED_G_PIN, 0);
      analogWrite(LED_B_PIN, 0);
    }
    lastSystemState = systemEnabled;
  }

  if (systemEnabled) {
    if (BP32.update()) {
      for (auto controller : myControllers) {
        if (controller && controller->isConnected() && controller->isGamepad()) {
          processarControleServo(controller);
          break;
        }
      }
    }

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      lerSensores();
      calcularEProcessarProbabilidade();
    }
  }
  
  delay(20);
}