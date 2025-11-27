#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>   // Biblioteca para controle de servo

const char* ssid = "ESP32";
const char* password = "12345678ab";

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_client_id = "ESP32Client";

const char* mqtt_topic_presence = "home/security/presence";
const char* mqtt_topic_motor_control = "home/security/motor/control";
const char* mqtt_topic_motor_status = "home/security/motor/status";
const char* mqtt_topic_alarm_status = "home/security/alarm/status";
const char* mqtt_topic_system_status = "home/security/system/status";

const int presenceSensorPin = 2;
const int buttonPin = 25;
const int ledPin = 4;
const int buzzerPin = 18;

bool systemActive = true;
bool presenceDetected = false;
bool motorOpen = false;

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[50];
int value = 0;

// --- Controle do Servo ---
Servo motorServo;          // Objeto Servo
const int servoPin = 13;   // Pino PWM do servo
const int posFechado = 0;  // Ângulo de porta/fechado
const int posAberto = 90;  // Ângulo de porta/aberto

void setup_wifi() {
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
  Serial.println("WiFi conectado");
  Serial.println("Endereco IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida [ ");
  Serial.print(topic);
  Serial.print(" ]: ");
  String message = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message += (char)payload[i];
  }
  Serial.println();

  if (String(topic) == mqtt_topic_motor_control) {
    if (message == "open" && !presenceDetected) {
      motorServo.write(posAberto);
      motorOpen = true;
      Serial.println("Motor abrindo...");
      client.publish(mqtt_topic_motor_status, "OPEN");
    } else if (message == "close" && !presenceDetected) {
      motorServo.write(posFechado);
      motorOpen = false;
      Serial.println("Motor fechando...");
      client.publish(mqtt_topic_motor_status, "CLOSED");
    } else if (presenceDetected) {
      Serial.println("Presenca detectada! Nao e possivel mover o motor.");
      client.publish(mqtt_topic_motor_status, "BLOCKED_PRESENCE");
    }
  }
}

long lastReconnectAttempt = 0;

void reconnect() {
  Serial.print("Tentando conexao MQTT...");
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String clientId = "ESP32Client-" + mac;
  Serial.print("Usando o ClientID: ");
  Serial.println(clientId);
  if (client.connect(clientId.c_str())) {
    Serial.println("conectado ao MQTT");
    client.subscribe(mqtt_topic_motor_control);
    client.publish(mqtt_topic_system_status, "ONLINE");
  } else {
    Serial.print("Falha na conexao MQTT, rc=");
    Serial.print(client.state());
    Serial.println(" tentando novamente em 5 segundos");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(presenceSensorPin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);

  digitalWrite(ledPin, LOW);
  noTone(buzzerPin); // garante buzzer desligado no inicio

  // Inicializa Servo
  motorServo.attach(servoPin);
  motorServo.write(posFechado);  // começa fechado

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      reconnect();
    }
  }
  client.loop();

  int presenceState = digitalRead(presenceSensorPin);
  if (presenceState == HIGH) {
    if (!presenceDetected) {
      presenceDetected = true;
      Serial.println("Presenca detectada!");
      client.publish(mqtt_topic_presence, "DETECTED");
      if (systemActive) {
        digitalWrite(ledPin, HIGH);
        tone(buzzerPin, 1000);  // Liga buzzer com frequência 1000 Hz
        client.publish(mqtt_topic_alarm_status, "ACTIVE");
        Serial.println("ALARME ATIVADO!");
      }
    }
  } else {
    if (presenceDetected) {
      presenceDetected = false;
      Serial.println("Nenhuma presenca detectada.");
      client.publish(mqtt_topic_presence, "CLEARED");
      if (systemActive) {
        digitalWrite(ledPin, LOW);
        noTone(buzzerPin);  // Desliga buzzer
        client.publish(mqtt_topic_alarm_status, "INACTIVE");
        Serial.println("ALARME DESATIVADO.");
      }
    }
  }

  if (digitalRead(buttonPin) == LOW) {
    if (systemActive) {
      systemActive = false;
      digitalWrite(ledPin, LOW);
      noTone(buzzerPin);
      Serial.println("Sistema de deteccao de invasao DESATIVADO manualmente.");
      client.publish(mqtt_topic_system_status, "DEACTIVATED");
      delay(1000);
    }
  } else {
    if (!systemActive && !presenceDetected) {
      systemActive = true;
      Serial.println("Sistema de deteccao de invasao ATIVADO automaticamente.");
      client.publish(mqtt_topic_system_status, "ACTIVE");
    }
  }

  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    client.publish("home/security/led/status", (digitalRead(ledPin) == HIGH) ? "ON" : "OFF");
    client.publish(mqtt_topic_motor_status, motorOpen ? "OPEN" : "CLOSED");
    client.publish(mqtt_topic_presence, presenceDetected ? "DETECTED" : "CLEARED");
    client.publish(mqtt_topic_system_status, systemActive ? "ACTIVE" : "DEACTIVATED");
  }

  delay(50);
}
