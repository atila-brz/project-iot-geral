ram#include <ESP32Servo.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// --- 📌 CONFIGURAÇÃO DE PINOS E SENSORES ---

const int servoPin = 26;
Servo meuServo;

const int buttonPin = 18;

const int potPin1 = 34;
const int potPin2 = 35;
const int ldrPin = 32;
const int LIMITE_LUZ = 2000; // 0 a 4095

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// --- 💡 CONFIGURAÇÃO DO LED RGB ---

const int pinR = 13;
const int pinG = 12;
const int pinB = 14; 

int rgbEstado = 0;

// --- ⏱️ VARIÁVEIS DE CONTROLE NÃO-BLOQUEANTE (MILLIS) ---

unsigned long tempoAnteriorDHT = 0;
const long intervaloDHT = 2000; // 2 segundos

unsigned long tempoAnteriorServo = 0;
const long intervaloServo = 1500; // 1.5 segundos
const int angulosServo[] = {0, 90, 180};
const int numAngulos = sizeof(angulosServo) / sizeof(angulosServo[0]);
int servoEstado = 0; 
int servoPos = 0; 

// --- 📊 VARIÁVEIS DE ESTADO (Para armazenar leituras) ---
int buttonState = HIGH;
int valorPot1 = 0;
int valorPot2 = 0;
int ldrValue = 0;
float dhtUmidade = 0.0;
float dhtTemperatura = 0.0;
float dhtSensacaoTermica = 0.0;


// ----------------------------------------------------------------
// --- ⚙️ FUNÇÕES DE LEITURA E ATUALIZAÇÃO ---
// ----------------------------------------------------------------

void lerInputs() {
  buttonState = digitalRead(buttonPin);
  valorPot1 = analogRead(potPin1);
  valorPot2 = analogRead(potPin2);
  ldrValue = analogRead(ldrPin);
}

void atualizarServo(unsigned long tempoAtual) {
  if (tempoAtual - tempoAnteriorServo >= intervaloServo) {
    tempoAnteriorServo = tempoAtual;

    servoEstado = (servoEstado + 1) % numAngulos;

    servoPos = angulosServo[servoEstado];

    meuServo.write(servoPos);
    
    atualizarRGB();
  }
}

void atualizarRGB() {
    rgbEstado = (rgbEstado + 1) % 4; 

    int r = 0, g = 0, b = 0;

    switch (rgbEstado) {
        case 0: 
            r = 0; g = 0; b = 0;
            break;
        case 1: 
            r = 255; g = 0; b = 0;
            break;
        case 2: 
            r = 0; g = 255; b = 0;
            break;
        case 3:
            r = 0; g = 0; b = 255;
            break;
    }

    analogWrite(pinR, r);
    analogWrite(pinG, g);
    analogWrite(pinB, b);
}

void lerDHT(unsigned long tempoAtual) {
  if (tempoAtual - tempoAnteriorDHT >= intervaloDHT) {
    tempoAnteriorDHT = tempoAtual; // Reinicia o timer

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println("\n[⚠️ DHT 4]: Falha na leitura do sensor DHT22!");
      Serial.println("--------------------------------------------------------------------------------");
    } else {
      dhtUmidade = h;
      dhtTemperatura = t;
      dhtSensacaoTermica = dht.computeHeatIndex(t, h, false);
      imprimirLeituraDHT(); // Imprime imediatamente após a leitura
    }
  }
}

// ----------------------------------------------------------------
// --- 🖥️ FUNÇÕES DE IMPRESSÃO SERIAL ---
// ----------------------------------------------------------------

void imprimirLeituras() {
  Serial.print("🤖 ");
  
  Serial.print("[BTN 18]: ");
  if (buttonState == LOW) {
    Serial.print("**PRESSIONADO**");
  } else {
    Serial.print("Livre");
  }

  Serial.print(" | [POT 34]: " + String(valorPot1) + " | [POT 35]: " + String(valorPot2));
  
  Serial.print(" | [LDR 32]: " + String(ldrValue));
  if (ldrValue > LIMITE_LUZ) {
    Serial.print(" (🔆 CLARO)");
  } else {
    Serial.print(" (🌑 ESCURO)");
  }
  
  Serial.print(" | [SERVO 26]: Posicao " + String(servoPos) + "°");
  Serial.print(" | [RGB " + String(pinR) + "," + String(pinG) + "," + String(pinB) + "]: Cor ");
  switch (rgbEstado) {
      case 0: Serial.println("OFF"); break;
      case 1: Serial.println("**VERMELHO**"); break;
      case 2: Serial.println("**VERDE**"); break;
      case 3: Serial.println("**AZUL**"); break;
  }
}

void imprimirLeituraDHT() {
  Serial.println("\n🌡️ --- LEITURA DO DHT22 (a cada 2s) ---");
  Serial.print("  | Umidade: **");
  Serial.print(dhtUmidade);
  Serial.print(" %**\t| Temperatura: **");
  Serial.print(dhtTemperatura);
  Serial.print(" °C** | Sensacao Termica: **");
  Serial.print(dhtSensacaoTermica);
  Serial.println(" °C**");
  Serial.println("--------------------------------------------------------------------------------");
}

// ----------------------------------------------------------------
// --- 🛠️ FUNÇÃO SETUP ---
// ----------------------------------------------------------------

void setup() {
  Serial.begin(115200);

  meuServo.attach(servoPin);
  meuServo.write(servoPos);

  pinMode(buttonPin, INPUT_PULLUP);

  dht.begin();
  
  pinMode(pinR, OUTPUT);
  pinMode(pinG, OUTPUT);
  pinMode(pinB, OUTPUT);
  
  atualizarRGB(); 

  Serial.println("--- 🟢 SISTEMA MULTISSENSOR ESP32 INICIADO ---");
  Serial.println("Pinos: Servo(26), Botao(18), Pots(34, 35), LDR(32), DHT(4)");
  Serial.println("LED RGB: R(" + String(pinR) + "), G(" + String(pinG) + "), B(" + String(pinB) + ")");
  Serial.println("--------------------------------------------------------------------------------");
}

// ----------------------------------------------------------------
// --- 🔄 FUNÇÃO LOOP ---
// ----------------------------------------------------------------

void loop() {
  unsigned long tempoAtual = millis();

  lerInputs();

  atualizarServo(tempoAtual);

  lerDHT(tempoAtual);

  imprimirLeituras();
  
  delay(10); 
}