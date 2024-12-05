#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "WiFi.h"


#define RXp2 16
#define TXp2 17
JsonDocument doc;
float batteryVoltage, current;
////////////////////////////////////////////////////////
/* Wifi Connection data                               */
/* Update these with values suitable for your network */
////////////////////////////////////////////////////////
const char* ssid = "";//use your ssid
const char* password = "";//use your password
const char* mqtt_server = "broker.mqtt-dashboard.com";
////////////////////////////////////
/* Broker Communication Variables */
////////////////////////////////////
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
const int maxReconnectionAttempts = 5;

// this sample code provided by www.programmingboss.com
void setup() {

  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, RXp2, TXp2);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Check for Message received on define topic for MQTT Broker
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

}

void reconnect() {
  // Loop until we're reconnected
  int attempts = 0;
  while (!client.connected() && attempts < maxReconnectionAttempts) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("VSTest", "Hello world");
      // ... and resubscribe
      client.subscribe("VSResponse");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      Serial2.println("Connection Lost");
      attempts++;
      // Wait 200 ms before retrying
      delay(200);
    }
  }
}

void loop() {

  while (1) {
    // put your main code here, to run repeatedly:
    if (!client.connected()) {
      reconnect();
    }
    if (!client.connected()) {
      delay(1000);
      Serial.println("Could not restablish connection");
      continue;
    }
    
    unsigned long now = millis();
    if (now - lastMsg > 2000){
    
      lastMsg = now;
      Serial.println("reading...");
      deserializeJson(doc, Serial2.readString());
      current = doc["Current"];
      batteryVoltage = doc["Voltage"];
      Serial.println("trying to publish");
      Serial.print("Current:");
      Serial.println(current);
      Serial.print("Battery Voltage:");
      Serial.println(batteryVoltage);
      
      snprintf (msg, MSG_BUFFER_SIZE, "Data: %g, %g", current, batteryVoltage);
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish("VSTest", msg);
      Serial2.println("Successfull read!");

      delay(1000);
    }

  }

}
