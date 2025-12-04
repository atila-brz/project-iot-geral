#include <WiFi.h>
#include <PubSubClient.h>
#include "BluetoothSerial.h"

#define COMMUNICATION_MODE 0 

const char* BT_DEVICE_NAME = "ESP32_Framework";
const char* WIFI_SSID = "Catatau";
const char* WIFI_PASSWORD = "ze colmeia";
const char* MQTT_SERVER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char* MQTT_PUB_TOPIC = "iot/framework/pub";
const char* MQTT_SUB_TOPIC = "iot/framework/sub";

volatile bool systemActive = true; 
unsigned long lastTelemetryTime = 0;
const unsigned long TELEMETRY_INTERVAL = 10000; 

WiFiClient espClient;
PubSubClient mqtt_client(espClient);
BluetoothSerial bt_serial;
char mqtt_clientId[30];

typedef void (*command_callback_t)(const char* data);
command_callback_t command_handler_callback = NULL;

struct CommInterface {
    bool (*begin)();
    bool (*is_connected)();
    void (*loop)();
    bool (*send)(const char* data);
};

struct CommInterface comm_interface = {0}; 

void handleIncomingCommand(const char* command);
void mqtt_connect_wifi();
void mqtt_reconnect();
void mqtt_callback_wrapper(char* topic, byte* payload, unsigned int length);
bool mqtt_begin();
bool mqtt_is_connected();
void mqtt_loop();
bool mqtt_send(const char* data);
bool bt_begin();
bool bt_is_connected();
void bt_loop();
bool bt_send(const char* data);

// -----------------------------------------------------------------------
// --- 6. IMPLEMENTAÇÕES CONCRETAS: MQTT (C-Style) ---
// -----------------------------------------------------------------------

void mqtt_connect_wifi() {
    Serial.print("MQTT: Conectando a rede: ");
    Serial.println(WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
        Serial.print(".");
        delay(500);
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nMQTT: Wi-Fi Conectado. IP: " + WiFi.localIP().toString());
    } else {
        Serial.println("\nMQTT: Falha ao conectar Wi-Fi.");
    }
}

void mqtt_reconnect() {
    char macStr[13];
    sprintf(macStr, "%02X%02X%02X%02X%02X%02X", 
            WiFi.macAddress()[0], WiFi.macAddress()[1], WiFi.macAddress()[2], 
            WiFi.macAddress()[3], WiFi.macAddress()[4], WiFi.macAddress()[5]);
    snprintf(mqtt_clientId, sizeof(mqtt_clientId), "ESP32Client-%s", macStr);
    
    if (mqtt_client.connect(mqtt_clientId)) {
        Serial.println("MQTT: Conectado ao Broker.");
        mqtt_client.subscribe(MQTT_SUB_TOPIC);
        Serial.print("MQTT: Subscrito ao tópico: ");
        Serial.println(MQTT_SUB_TOPIC);
    } else {
        Serial.print("MQTT: Falha na conexão (RC=");
        Serial.print(mqtt_client.state());
        Serial.println("). Tentando novamente...");
    }
}

void mqtt_callback_wrapper(char* topic, byte* payload, unsigned int length) {
    payload[length] = '\0'; 
    
    Serial.print("MQTT: Mensagem recebida [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.println((char*)payload);
    
    if (command_handler_callback) {
        command_handler_callback((const char*)payload);
    }
}

bool mqtt_begin() {
    mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
    mqtt_client.setCallback(mqtt_callback_wrapper);

    mqtt_connect_wifi();
    if (WiFi.status() == WL_CONNECTED) {
        mqtt_reconnect();
        return true;
    }
    return false;
}

bool mqtt_is_connected() {
    return mqtt_client.connected();
}

void mqtt_loop() {
    static long lastReconnectAttempt = 0;
    if (WiFi.status() == WL_CONNECTED && !mqtt_client.connected()) { // Verifica Wi-Fi antes de tentar MQTT
        long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            mqtt_reconnect();
        }
    }
    mqtt_client.loop();
}

bool mqtt_send(const char* data) {
    if (!mqtt_client.connected()) {
        Serial.println("MQTT: Falha ao publicar, cliente desconectado.");
        return false;
    }
    Serial.print("MQTT: Publicando em ");
    Serial.print(MQTT_PUB_TOPIC);
    Serial.print(": ");
    Serial.println(data);
    return mqtt_client.publish(MQTT_PUB_TOPIC, data);
}

// -----------------------------------------------------------------------
// --- 7. IMPLEMENTAÇÕES CONCRETAS: BLUETOOTH (C-Style) ---
// -----------------------------------------------------------------------

bool bt_begin() {
    Serial.println("BLUETOOTH: Iniciando Bluetooth...");
    if (!bt_serial.begin(BT_DEVICE_NAME)) {
        Serial.println("BLUETOOTH: ERRO ao inicializar o Bluetooth.");
        return false;
    } else {
        Serial.print("BLUETOOTH: Iniciado com sucesso. Nome: ");
        Serial.println(BT_DEVICE_NAME);
        return true;
    }
}

bool bt_is_connected() {
    return bt_serial.hasClient();
}

void bt_loop() {
    static unsigned long lastStatusCheck = 0;
    if (millis() - lastStatusCheck > 5000) {
        if (bt_serial.hasClient()) {
            Serial.println("BLUETOOTH STATUS: Conectado a um dispositivo.");
        } else {
            Serial.println("BLUETOOTH STATUS: Aguardando conexão...");
        }
        lastStatusCheck = millis();
    }
    
    // Simplificação da leitura: lê linha por linha, foca no callback.
    if (bt_serial.available()) {
        String message = bt_serial.readStringUntil('\n'); 
        
        Serial.print("BLUETOOTH: Mensagem recebida: ");
        Serial.println(message);

        // Converte a String C++ para C-string (char*) para o callback
        if (command_handler_callback) {
            command_handler_callback(message.c_str());
        }
    }
}

bool bt_send(const char* data) {
    if (bt_serial.hasClient()) {
        Serial.print("BLUETOOTH: Enviando: ");
        Serial.println(data);
        bt_serial.println(data); 
        return true;
    }
    Serial.println("BLUETOOTH: Falha ao enviar, nenhum cliente conectado.");
    return false;
}

// -----------------------------------------------------------------------
// --- 8. LÓGICA PRINCIPAL (Core do Framework) ---
// -----------------------------------------------------------------------

void handleIncomingCommand(const char* command) {
    Serial.print("LÓGICA: Processando comando '");
    Serial.print(command);
    Serial.println("'...");

    String cmd = command;

    if (cmd == "TOGGLE_STATE") {
        systemActive = !systemActive;
        const char* status = systemActive ? "ATIVO" : "INATIVO";
        Serial.println("LÓGICA: Estado do sistema alterado para " + String(status));
        
        char response[50];
        snprintf(response, 50, "STATUS:%s", status);
        comm_interface.send(response); 
    } else if (cmd == "PING") {
        const char* status = systemActive ? "True" : "False";
        char response[50];
        snprintf(response, 50, "PONG:System Active=%s", status);
        comm_interface.send(response);
    }
}

// -----------------------------------------------------------------------
// --- 5. SETUP E LOOP (Core do Framework) ---
// -----------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- INICIALIZAÇÃO DO FRAMEWORK IOT MODULAR (C-STYLE) ---");

    command_handler_callback = handleIncomingCommand;

    if (COMMUNICATION_MODE == 0) {
        Serial.println("Modo Selecionado: MQTT via Wi-Fi");
        comm_interface.begin = mqtt_begin;
        comm_interface.is_connected = mqtt_is_connected;
        comm_interface.loop = mqtt_loop;
        comm_interface.send = mqtt_send;
    } else if (COMMUNICATION_MODE == 1) {
        Serial.println("Modo Selecionado: Bluetooth Serial");
        comm_interface.begin = bt_begin;
        comm_interface.is_connected = bt_is_connected;
        comm_interface.loop = bt_loop;
        comm_interface.send = bt_send;
    } else {
        Serial.println("ERRO: Modo de comunicação inválido. Desligando...");
        while (true) delay(100); 
    }
    
    if (comm_interface.begin) {
        comm_interface.begin();
    }
    
    Serial.println("SETUP CONCLUÍDO.");
}

void loop() {
    if (comm_interface.loop) {
        comm_interface.loop();
    }

    if (systemActive && comm_interface.is_connected && comm_interface.is_connected() && millis() - lastTelemetryTime > TELEMETRY_INTERVAL) {
        lastTelemetryTime = millis();
        
        char telemetry[100];
        snprintf(telemetry, 100, "TELEMETRIA:Timestamp=%lu,Value=42.0", lastTelemetryTime);
        
        comm_interface.send(telemetry);
    }
}
