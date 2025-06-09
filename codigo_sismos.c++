#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

#define DHTPIN 15
#define DHTTYPE DHT22


const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqtt_server = "broker.hivemq.com"; // ou IP local do Node-RED
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

DHT dht(DHTPIN, DHTTYPE);

const char* mqtt_topic_temp = "safezone/temperatura";

void setup_wifi() {
  Serial.print("Conectando-se a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nWiFi conectado! IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Conectando ao broker MQTT...");
    if (client.connect("esp32-safezone-client")) {
      Serial.println("conectado!");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  // Lê a temperatura e envia a cada 5 segundos
  static unsigned long lastSent = 0;
  if (millis() - lastSent > 5000) {
    float temp = dht.readTemperature();

    if (!isnan(temp)) {
      char temp_str[10];
      dtostrf(temp, 4, 2, temp_str);
      client.publish(mqtt_topic_temp, temp_str);
      Serial.print("Temperatura enviada: ");
      Serial.println(temp_str);
    } else {
      Serial.println("Erro na leitura do DHT22");
    }

    lastSent = millis();
  }
}
