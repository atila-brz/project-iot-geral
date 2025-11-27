#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h> 

const char* SSID = "ZEGAneT"; 
const char* PASSWORD = "12345678"; 

const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC_COMANDO = "robô/movimento/comando";

const int SERVO_ESQ_PIN = 33;
const int SERVO_DIR_PIN = 12;

const int POS_NEUTRA = 90;
const int POS_AVANTE = 135;
const int POS_RE = 45;

Servo servoEsq;
Servo servoDir;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void pararServos();
void moverFrente();
void moverTras();
void virarDireita();
void virarEsquerda();

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);

  String comando;
  for (int i = 0; i < length; i++) {
    comando += (char)payload[i];
  }
  Serial.print("Comando: ");
  Serial.println(comando);

  if (comando.equalsIgnoreCase("FRENTE")) {
    moverFrente();
  } else if (comando.equalsIgnoreCase("TRAS")) {
    moverTras();
  } else if (comando.equalsIgnoreCase("DIREITA")) {
    virarDireita();
  } else if (comando.equalsIgnoreCase("ESQUERDA")) {
    virarEsquerda();
  } else if (comando.equalsIgnoreCase("PARAR")) {
    pararServos();
  } else {
    Serial.println("Comando desconhecido.");
    pararServos();
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando-se a ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
  while (!mqttClient.connected()) {
    Serial.print("Tentando conectar ao MQTT Broker...");
    if (mqttClient.connect("ESP32_Robo_Servo_Cliente")) {
      Serial.println("Conectado!");
      mqttClient.subscribe(MQTT_TOPIC_COMANDO);
      Serial.print("Subscrito ao tópico: ");
      Serial.println(MQTT_TOPIC_COMANDO);
    } else {
      Serial.print("falhou, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  servoEsq.attach(SERVO_ESQ_PIN);
  servoDir.attach(SERVO_DIR_PIN);

  pararServos();
  Serial.println("Pinos dos servos configurados. Servos em posição neutra.");

  setup_wifi();
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(callback);
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect_mqtt();
  }
  mqttClient.loop();
}

void pararServos() {
  servoEsq.write(POS_NEUTRA);
  servoDir.write(POS_NEUTRA);
  Serial.println("Ação: Parar (Posição Neutra)");
}

void moverFrente() {
  servoEsq.write(POS_AVANTE);
  servoDir.write(POS_AVANTE);
  Serial.println("Ação: Mover para Frente");
}

void moverTras() {
  servoEsq.write(POS_RE);
  servoDir.write(POS_RE);
  Serial.println("Ação: Mover para Trás");
}

void virarDireita() {
  servoEsq.write(POS_AVANTE); 
  servoDir.write(POS_RE);    
  Serial.println("Ação: Virar à Direita");
}

void virarEsquerda() {
  servoEsq.write(POS_RE);     // Esquerdo recua
  servoDir.write(POS_AVANTE); // Direito avança
  Serial.println("Ação: Virar à Esquerda");
}