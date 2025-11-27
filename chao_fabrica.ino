/**
 * @file chao_fabrica.ino
 * @brief Implementação do controle de produção com múltiplos sensores e atuadores em um ESP32.
 *
 * Funcionalidades:
 * - Controle de produção via botão de interrupção.
 * - Controle de velocidade de oscilação de dois motores por potenciômetros.
 * - Monitoramento de segurança (temperatura) que para a produção.
 * - Relatório de status periódico no Monitor Serial.
 */

// ----- INCLUSÃO DE BIBLIOTECAS -----
#include <ESP32Servo.h>
#include <DHT.h>

// ----- MAPEAMENTO DE PINOS -----
#define MT1_PIN 13
#define MT2_PIN 19
#define BUZZER_PIN 5
#define LED_VERM_PIN 2
#define LED_VERD_PIN 4
#define SENSOR_TEMP_PIN 18
#define INC_PIN 33
#define INTERRUPTOR_PIN 14
#define POT1_PIN 25 
#define POT2_PIN 32

// ----- CONSTANTES E CONFIGURAÇÕES -----
#define DHT_TYPE DHT22

// ----- OBJETOS GLOBAIS -----
Servo motor1;
Servo motor2;
DHT dht(SENSOR_TEMP_PIN, DHT_TYPE);

// ----- VARIÁVEIS DE ESTADO E CONTROLE -----
volatile bool paradaManual = false;
volatile bool paradaManualAcionada = false;

struct {
    bool temperaturaCritica = false;
    bool inclinacaoIncorreta = false;
    // A flag paradaManual foi movida para fora da struct para poder ser declarada como volatile.
} motivosParada;

int velocidadeMotor1 = 0; // Agora representa a "configuração" de velocidade (0-180)
int velocidadeMotor2 = 0;

// Novas variáveis para controle de oscilação
int posMotor1 = 90; // Posição atual do motor 1 (começa no meio)
int dirMotor1 = 1;  // Direção (1 = 0->180, -1 = 180->0)
unsigned long tPrevMotor1 = 0; // Último tempo que o motor 1 se moveu

int posMotor2 = 90; // Posição atual do motor 2
int dirMotor2 = 1;  // Direção do motor 2
unsigned long tPrevMotor2 = 0; // Último tempo que o motor 2 se moveu


// ----- VARIÁVEIS DO TEMPORIZADOR -----
unsigned long previousMillis = 0;
const long interval = 2000;

// ----- PROTÓTIPOS DE FUNÇÕES -----
void getSensorTemperaturaUmidade(float &temperatura, float &umidade);
void imprimirStatus();
void IRAM_ATTR ISR_botao();

// ----- FUNÇÃO DE INTERRUPÇÃO -----
void IRAM_ATTR ISR_botao() {
    paradaManual = !paradaManual;
    if (paradaManual) {
        paradaManualAcionada = true; // Indica que a parada foi acionada manualmente
    }
}

// ----- FUNÇÕES DE LEITURA DE SENSORES (MODULARES) -----

/**
 * @brief Lê o sensor DHT22 e atualiza as variáveis de temperatura e umidade.
 * @param temperatura Referência para a variável de temperatura.
 * @param umidade Referência para a variável de umidade.
 */
void getSensorTemperaturaUmidade(float &temperatura, float &umidade) {
    umidade = dht.readHumidity();
    temperatura = dht.readTemperature();
}

// ----- FUNÇÃO DE STATUS -----

/**
 * @brief Imprime o status completo do sistema no Monitor Serial.
 */
void imprimirStatus() {
    float temperatura, umidade;
    getSensorTemperaturaUmidade(temperatura, umidade);
    bool paradaAutomatica = motivosParada.temperaturaCritica || motivosParada.inclinacaoIncorreta;

    Serial.println("------------------------------------\n");
    Serial.println("RELATÓRIO DE STATUS");
    
    // Status da Produção
    Serial.print("Status da Produção: ");
    if (!paradaManual && !paradaAutomatica) {
        Serial.println("ATIVA");
    } else {
        Serial.println("PARADA");
    }

    // Status dos Sensores
    if (isnan(temperatura)) {
        Serial.println("Sensor de Temperatura: Falha na leitura");
    } else {
        Serial.printf("Sensor de Temperatura: %.1f °C\n", temperatura);
    }

    // Velocidade dos Motores (mostra a configuração 0-180 do potenciômetro)
    Serial.printf("Velocidade Motor 1 (Vertical): %d\n", velocidadeMotor1);
    Serial.printf("Velocidade Motor 2 (Horizontal): %d\n", velocidadeMotor2);
    Serial.printf("Sensor de Inclinação: %d\n", digitalRead(INC_PIN));
    Serial.println("------------------------------------\n");
}


// ----- FUNÇÃO DE CONFIGURAÇÃO INICIAL (SETUP) -----
void setup() {
    Serial.begin(115200);
    Serial.println("\n--- INICIANDO SISTEMA DE CONTROLE DE PRODUÇÃO ---");

    // Configuração dos pinos de saída
    pinMode(LED_VERD_PIN, OUTPUT);
    pinMode(LED_VERM_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    // Configuração dos pinos de entrada
    pinMode(INTERRUPTOR_PIN, INPUT_PULLUP);
    pinMode(INC_PIN, INPUT_PULLUP);
    pinMode(POT1_PIN, INPUT);
    pinMode(POT2_PIN, INPUT);

    // Inicialização dos sensores
    dht.begin();

    // Anexa os servos aos seus pinos.
    motor1.attach(MT1_PIN);
    motor2.attach(MT2_PIN);

    // Garante que os motores comecem na posição central (90 graus)
    motor1.write(posMotor1);
    motor2.write(posMotor2);

    // Configuração da interrupção
    attachInterrupt(digitalPinToInterrupt(INTERRUPTOR_PIN), ISR_botao, FALLING);

    Serial.println("Setup finalizado. Sistema pronto.");
    delay(1000);
}

// ----- FUNÇÃO PRINCIPAL (LOOP) -----
void loop() {
    // Pega o tempo atual UMA VEZ no início do loop
    unsigned long currentMillis = millis();

    // --- Bloco de Verificação de Segurança ---
    float temperatura, umidade;
    getSensorTemperaturaUmidade(temperatura, umidade);
    motivosParada.temperaturaCritica = (!isnan(temperatura) && (temperatura < 20.0 || temperatura > 30.0));

    // Leitura do sensor de inclinação (digital)
    // Assume-se que LOW (0) significa que está inclinado/fora do eixo
    motivosParada.inclinacaoIncorreta = (digitalRead(INC_PIN) == HIGH);

    bool algumaParadaAutomatica = motivosParada.temperaturaCritica || motivosParada.inclinacaoIncorreta;
     
    // --- Bloco de Controle da Produção ---
    if (paradaManual) {
        // ESTADO DE PARADA MANUAL
        digitalWrite(LED_VERD_PIN, LOW);
        digitalWrite(LED_VERM_PIN, HIGH);
        noTone(BUZZER_PIN); // Garante que o buzzer esteja desligado na parada manual

        // Para os motores e reseta o estado
        velocidadeMotor1 = 0;
        velocidadeMotor2 = 0;
        motor1.write(90);
        motor2.write(90);
        posMotor1 = 90;
        posMotor2 = 90;
        dirMotor1 = 1;
        dirMotor2 = 1;

        if (paradaManualAcionada) { // Mostra a mensagem apenas uma vez ao acionar a parada
            Serial.println("PRODUÇÃO PARADA PELO OPERADOR.");
            paradaManualAcionada = false; // Reseta a flag após mostrar a mensagem
        }

    } else if (algumaParadaAutomatica) {
        // ESTADO DE PARADA AUTOMÁTICA (SEGURANÇA)
        digitalWrite(LED_VERD_PIN, LOW);
        digitalWrite(LED_VERM_PIN, HIGH);
        
        // Para os motores e reseta o estado
        velocidadeMotor1 = 0;
        velocidadeMotor2 = 0;
        motor1.write(90);
        motor2.write(90);
        posMotor1 = 90;
        posMotor2 = 90;
        dirMotor1 = 1;
        dirMotor2 = 1;

        // Imprime o motivo da parada automática e ativa o alarme
        if (motivosParada.temperaturaCritica) {
            Serial.println("ALERTA: Temperatura Crítica! Produção parada.");
        }
        if (motivosParada.inclinacaoIncorreta) {
            Serial.println("ALERTA: Madeira fora do Eixo. Produção parada.");
        }
        tone(BUZZER_PIN, 2000); // Alarme sonoro

    } else {
        // ESTADO DE PRODUÇÃO NORMAL
        digitalWrite(LED_VERD_PIN, HIGH);
        digitalWrite(LED_VERM_PIN, LOW);
        noTone(BUZZER_PIN);
        
        // 1. Lê os potenciômetros e mapeia para a faixa de "velocidade" (0-180)
        velocidadeMotor1 = map(analogRead(POT1_PIN), 0, 4095, 0, 180);
        velocidadeMotor2 = map(analogRead(POT2_PIN), 0, 4095, 0, 180);

        // 2. Mapeia a "velocidade" para um intervalo de tempo (em ms)
        long intervaloMotor1 = map(velocidadeMotor1, 0, 180, 50, 5);
        long intervaloMotor2 = map(velocidadeMotor2, 0, 180, 50, 5);

        // --- Lógica de Oscilação Motor 1 ---
        if (currentMillis - tPrevMotor1 >= intervaloMotor1) {
            tPrevMotor1 = currentMillis;
            posMotor1 = posMotor1 + dirMotor1;
            motor1.write(posMotor1);
            if (posMotor1 >= 180) {
                dirMotor1 = -1;
            } else if (posMotor1 <= 0) {
                dirMotor1 = 1;
            }
        }

        // --- Lógica de Oscilação Motor 2 ---
        if (currentMillis - tPrevMotor2 >= intervaloMotor2) {
            tPrevMotor2 = currentMillis;
            posMotor2 = posMotor2 + dirMotor2;
            motor2.write(posMotor2);
            if (posMotor2 >= 180) {
                dirMotor2 = -1;
            } else if (posMotor2 <= 0) {
                dirMotor2 = 1;
            }
        }
    }

    // --- Bloco do Temporizador de Status ---
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        imprimirStatus();
    }
    
    // O delay(100) foi removido para não bloquear a oscilação dos motores.
}
