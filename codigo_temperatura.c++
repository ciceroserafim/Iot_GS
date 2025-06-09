// Inclui as bibliotecas necessárias para Wi-Fi e MQTT
#include <WiFi.h>
#include <PubSubClient.h>

// --- Configurações de Wi-Fi ---
const char* ssid = "Wokwi-GUEST"; // SSID da rede Wi-Fi (Wokwi usa "Wokwi-GUEST")
const char* password = "";        // Senha da rede Wi-Fi (Wokwi não precisa de senha)

// --- Configurações de MQTT ---
const char* mqtt_server = "broker.hivemq.com"; // Endereço do broker MQTT público para simulação
const int mqtt_port = 1883;                    // Porta padrão do MQTT
const char* mqtt_client_id = "esp32-safezone-client"; // ID único para o seu cliente MQTT

// --- Tópicos MQTT Personalizados e Únicos ---
const char* mqtt_topic_sismo = "safezone/sensor/sismo";
const char* mqtt_topic_led_control = "safezone/atuador/led_alerta"; // Tópico para controle do LED

// --- Configurações dos Pinos ---
#define BUTTON_PIN 4 // GPIO4 = Pino para o botão de vibração (simula sismo)
#define LED_PIN 2    // GPIO2 = Pino para o LED de alerta (LED onboard do ESP32 no Wokwi)

// --- Objetos de Cliente Wi-Fi e MQTT ---
WiFiClient espClient;
PubSubClient client(espClient);

// --- Função para conectar ao Wi-Fi ---
void setup_wifi() {
  Serial.print("Conectando-se a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado! IP: " + WiFi.localIP().toString());
}

// --- Função de Callback para mensagens MQTT recebidas ---
// Esta função é chamada quando o ESP32 recebe uma mensagem em um tópico que ele assina
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.printf("Mensagem no tópico %s: %s\n", topic, message.c_str());

  // Controlar o LED com base na mensagem recebida
  if (String(topic) == mqtt_topic_led_control) {
    if (message == "ON") {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED LIGADO!");
    } else if (message == "OFF") {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED DESLIGADO!");
    }
  }
}

// --- Função para reconectar ao broker MQTT ---
void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Tentando conectar ao MQTT...");
    // Tenta conectar com o ID do cliente
    if (client.connect(mqtt_client_id)) {
      Serial.println("conectado!");
      // Uma vez conectado, assina o tópico de controle do LED
      client.subscribe(mqtt_topic_led_control);
    } else {
      Serial.printf("falhou, rc=%d. Tentando novamente em 5s...\n", client.state());
      delay(5000);
    }
  }
}

// --- Função de Configuração (Executada uma vez no início) ---
void setup() {
  Serial.begin(115200); // Inicia a comunicação serial para depuração
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Configura o pino do botão como entrada com pull-up interno
  pinMode(LED_PIN, OUTPUT);          // Configura o pino do LED como saída

  setup_wifi();                      // Conecta ao Wi-Fi
  client.setServer(mqtt_server, mqtt_port); // Configura o servidor MQTT
  client.setCallback(callback);             // Define a função de callback para mensagens recebidas
}

// --- Função Principal (Executada repetidamente) ---
void loop() {
  if (!client.connected()) {
    reconnect_mqtt(); // Garante que a conexão MQTT esteja ativa
  }
  client.loop(); // Processa mensagens MQTT recebidas e mantém a conexão

  // Leitura do estado do botão para simular a detecção de sismo
  static bool lastButtonState = HIGH; // Estado anterior do botão (HIGH = não pressionado)
  bool currentButtonState = digitalRead(BUTTON_PIN); // Estado atual do botão

  // Detecta a transição de não pressionado (HIGH) para pressionado (LOW)
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    Serial.println("Sismo detectado! Enviando alerta MQTT...");
    // Publica a mensagem JSON para indicar sismo
    client.publish(mqtt_topic_sismo, "{\"sismo\": true}");
    // Opcional: Ligar o LED ao detectar sismo
    digitalWrite(LED_PIN, HIGH);
  } else if (lastButtonState == LOW && currentButtonState == HIGH) {
    // Opcional: Ligar o LED ao detectar sismo
    digitalWrite(LED_PIN, LOW);
  }

  lastButtonState = currentButtonState; // Atualiza o estado anterior do botão
  delay(100); // Pequeno atraso para evitar leituras múltiplas rápidas do botão
}
