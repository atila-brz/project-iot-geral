#include <WiFi.h>
#include <PubSubClient.h>
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>

// =================================================================
// 1. CONFIGURAÇÕES DA MATRIZ (DISPLAY)
// =================================================================
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 1
#define CS_PIN 14   // Seu pino CS definido no código

MD_Parola display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// =================================================================
// 2. CONFIGURAÇÕES DE REDE E MQTT
// =================================================================
const char* ssid = "Catatau";      // <--- COLOQUE AQUI
const char* password = "ze colmeia"; // <--- COLOQUE AQUI

// Broker público para testes
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// Tópicos
const char* mqtt_topic_subscribe = "esp32/display/letra"; 

WiFiClient espClient;
PubSubClient client(espClient);
String mqtt_client_id; 

// =================================================================
// 3. FUNÇÃO DE CALLBACK (RECEBE A MENSAGEM)
// =================================================================
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  
  // Converte os bytes recebidos para String
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  // --- ATUALIZA O DISPLAY ---
  // Limpa o display e imprime a nova mensagem/letra
  display.displayClear();
  
  // Centraliza novamente (garantia)
  display.setTextAlignment(PA_CENTER);
  
  // Exibe a mensagem recebida
  display.print(message);
}

// =================================================================
// 4. FUNÇÕES DE CONEXÃO
// =================================================================
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando em ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop até conectar
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    
    // Tenta conectar
    if (client.connect(mqtt_client_id.c_str())) {
      Serial.println("conectado!");
      
      // Se inscreve no tópico para ouvir as letras
      client.subscribe(mqtt_topic_subscribe);
      Serial.print("Inscrito no tópico: ");
      Serial.println(mqtt_topic_subscribe);
      
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tente novamente em 5s");
      delay(5000);
    }
  }
}

// =================================================================
// 5. SETUP
// =================================================================
void setup() {
  Serial.begin(115200);

  // ---- INICIALIZA DISPLAY ----
  display.begin();
  display.setIntensity(5); // Brilho (0-15)
  display.displayClear();
  display.setTextAlignment(PA_CENTER);
  display.print("?"); // Mostra ? enquanto não conecta

  // ---- INICIALIZA WIFI ----
  setup_wifi();

  // ---- INICIALIZA MQTT ----
  // Gera ID único baseado no MAC para não dar conflito no broker
  mqtt_client_id = "ESP32Matrix-";
  mqtt_client_id += WiFi.macAddress();
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// =================================================================
// 6. LOOP
// =================================================================
void loop() {
  // Mantém conexão MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // ESSENCIAL para receber mensagens

  // A biblioteca Parola precisa disso para animações, 
  // mesmo usando print estático é bom manter.
  if (display.displayAnimate()) {
    // Nada a fazer aqui se for só estático, mas a função deve rodar.
  }
}