#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <secrets.h>

// MQTT topics

const char *pubTopic = "esp32/pub";
const char *subTopic = "esp32/sub";

void setupWifi();
void setupCertificates();
void setupMqtt();
void callback(char *topic, byte *payload, unsigned int length);

// Cliente MQTT

WiFiClientSecure net;
PubSubClient client(net);

const int LED_PIN = 23;

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);

  setupWifi();

  setupCertificates();

  setupMqtt();
}

void setupWifi() {
  delay(10);

  Serial.println();

  Serial.print("Conectando a ");

  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);

    Serial.print(".");
  }

  Serial.println("");

  Serial.println("WiFi conectado");

  Serial.print("Endereço IP: ");

  Serial.println(WiFi.localIP());
}

void setupCertificates() {
  net.setCACert(AWS_CERT_CA);

  net.setCertificate(AWS_CERT_CRT);

  net.setPrivateKey(AWS_CERT_PRIVATE);
}

void callback(char *topic, byte *payload, unsigned int length) {
  // Alocar um buffer para o payload
  char *message = (char *)malloc(length + 1);

  memcpy(message, payload, length);

  message[length] = '\0'; // Adicionar terminador nulo

  Serial.print("Mensagem recebida [");

  Serial.print(topic);

  Serial.print("]: ");
  Serial.println(message);

  // Alocar um buffer para o JSON
  JsonDocument doc;

  // Analisar o JSON
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("Erro ao analisar JSON: ");

    Serial.println(error.c_str());
  } else {
    // Verificar o comando

    const char *command = doc["message"];

    if (strcmp(command, "turn-on") == 0) {

      digitalWrite(LED_PIN, HIGH);

      // Enviar status em JSON

      JsonDocument statusDoc;

      statusDoc["status"] = "on";

      char statusMessage[100];

      serializeJson(statusDoc, statusMessage);

      client.publish(pubTopic, statusMessage);
    } else if (strcmp(command, "turn-off") == 0) {
      digitalWrite(LED_PIN, LOW);

      // Enviar status em JSON

      JsonDocument statusDoc;

      statusDoc["status"] = "off";

      char statusMessage[100];

      serializeJson(statusDoc, statusMessage);

      client.publish(pubTopic, statusMessage);
    } else {
      Serial.println("Comando desconhecido");
    }
  }

  // Liberar a memória alocada

  free(message);
}

void setupMqtt() {
  client.setServer(AWS_IOT_ENDPOINT, 8883);

  client.setCallback(callback);
}

void reconnect() {
  while (!client.connected()) {

    Serial.print("Tentando conectar ao MQTT...");

    if (client.connect(THINGNAME)) {

      Serial.println("Conectado");

      client.subscribe(subTopic);
    } else {

      Serial.print("Falha, rc=");

      Serial.print(client.state());

      Serial.println(" Tentando novamente em 5 segundos");

      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();
}
