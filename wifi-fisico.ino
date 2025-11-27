#include <WiFi.h>

// --- Configurações da Rede Wi-Fi ---
#define WIFI_SSID "José Gabriel's Galaxy S21 FE 5G"
#define WIFI_PASSWORD "12345678"
#define WIFI_CHANNEL 6

// --- Configurações dos LEDs ---
#define LED_VERMELHO_PIN 2  // LED Vermelho na porta GPIO 2
#define LED_VERDE_PIN 4     // LED Verde na porta GPIO 4

// --- Protótipos das Funções ---
void scanNetworks();
void connectToWiFi();

void setup() {
  Serial.begin(115200);
  Serial.println("\nInicializando o ESP32...");

  // Configura os pinos dos LEDs como OUTPUT
  pinMode(LED_VERMELHO_PIN, OUTPUT);
  pinMode(LED_VERDE_PIN, OUTPUT);

  // Define o modo Wi-Fi como Station (cliente)
  WiFi.mode(WIFI_STA);
  
  // Desconecta de qualquer rede anterior para um início limpo
  WiFi.disconnect();
  delay(100);

  Serial.println("Setup concluído!");
  Serial.print("Endereço MAC da placa: ");
  Serial.println(WiFi.macAddress());

  // Inicialmente, acende o LED vermelho (não conectado) e apaga o verde
  digitalWrite(LED_VERMELHO_PIN, HIGH);
  digitalWrite(LED_VERDE_PIN, LOW);
}

void loop() {
  // 1. Escaneia as redes Wi-Fi disponíveis
  scanNetworks();

  // 2. Tenta se conectar à rede definida
  connectToWiFi();

  // 3. Se conectado, entra em um loop vazio para manter a conexão.
  //    Caso contrário, o ESP reiniciará dentro da função connectToWiFi.
  Serial.println("\nConexão estabelecida. O dispositivo permanecerá conectado.");
  Serial.println("Aguardando 30 segundos antes de escanear novamente...");
  delay(30000); // Aguarda 30 segundos antes de repetir o processo
}

/**
 * @brief Escaneia as redes Wi-Fi disponíveis e exibe no Serial Monitor.
 */
void scanNetworks() {
  Serial.println("\nIniciando escaneamento de redes...");
  int n = WiFi.scanNetworks();
  Serial.println("Escaneamento concluído!");

  if (n == 0) {
    Serial.println("Nenhuma rede encontrada.");
  } else {
    Serial.print(n);
    Serial.println(" redes encontradas:");
    for (int i = 0; i < n; ++i) {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (RSSI: ");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      // Adiciona um "*" para redes protegidas por senha
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "" : " *");
      delay(10);
    }
  }
  Serial.println("");
}

/**
 * @brief Conecta-se à rede Wi-Fi especificada nas macros.
 *        Se a conexão falhar após 10 segundos, o dispositivo reinicia.
 */
void connectToWiFi() {
  Serial.print("Conectando à rede: ");
  Serial.println(WIFI_SSID);

  // Garante que o LED vermelho esteja aceso e o verde apagado durante a tentativa de conexão
  digitalWrite(LED_VERMELHO_PIN, HIGH);
  digitalWrite(LED_VERDE_PIN, LOW);

  // Inicia a conexão. O canal é um parâmetro opcional que pode acelerar o processo.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);

  unsigned long startAttemptTime = millis();

  // Aguarda a conexão por até 10 segundos
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFalha na conexão! Reiniciando o dispositivo...");
    // Mantém o LED vermelho aceso e o verde apagado antes de reiniciar
    digitalWrite(LED_VERMELHO_PIN, HIGH);
    digitalWrite(LED_VERDE_PIN, LOW);
    delay(2000);
    ESP.restart();
  } else {
    Serial.println("\nConexão bem-sucedida!");
    Serial.print("Endereço IP obtido: ");
    Serial.println(WiFi.localIP());
    // Conexão bem-sucedida: apaga o LED vermelho e acende o verde
    digitalWrite(LED_VERMELHO_PIN, LOW);
    digitalWrite(LED_VERDE_PIN, HIGH);
  }
}