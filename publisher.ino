#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "WiFi.h"
#include <time.h>

#define RXp2 16
#define TXp2 17
long timezone = -3;
byte daysavetime = 1;
struct tm timeInfo;
JsonDocument doc;
float batteryVoltage, current;
////////////////////////////////////////////////////////
/* Wifi Connection data                               */
/* Update these with values suitable for your network */
////////////////////////////////////////////////////////
const char* ssid = "";//use your ssid
const char* password = "";//use your password
const char* mqtt_server = "broker.mqtt-dashboard.com";
const int mqtt_port = 1883;
const int MQTT_BUFFER_LENGTH=500;
const char* MQTT_TOPIC = "test";
////////////////////////////////////
/* Broker Communication Variables */
////////////////////////////////////
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
const int maxReconnectionAttempts = 5;
const int SERIALBR = 9600;
char timeBuff[30];

// this sample code provided by www.programmingboss.com
void setup() {
  Serial.begin(SERIALBR);
  Serial2.begin(SERIALBR, SERIAL_8N1, RXp2, TXp2);
  setup_wifi();
  setDateTime();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}
  
void setDateTime() {
  // You can use your own timezone, but the exact time is not used at all.
  // Only the date is needed for validating the certificates.
  configTime(3600 * timezone, daysavetime * 3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
  // America/Sao_Paulo, "pool.ntp.org", "time.nist.gov"

  Serial.print("Waiting for NTP time sync: ");
  time_t nowe = time(nullptr);
  while (nowe < 8 * 3600 * 2) {
    delay(100);
    Serial.print(".");
    nowe = time(nullptr);
  }
  Serial.println();

  struct tm timeinfo;
  gmtime_r(&nowe, &timeinfo);
  Serial.printf("%s %s", tzname[0], asctime(&timeinfo));
}

void updateTimeBuff() {
  getLocalTime(&timeInfo);
  snprintf(timeBuff,30,"%d-%d-%dT%d:%d:%dZ-0300",timeInfo.tm_year+1900,timeInfo.tm_mon+1,timeInfo.tm_mday,timeInfo.tm_hour,timeInfo.tm_min,timeInfo.tm_sec);
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

void sendMQTT(float voltage, float current, const char* t, const char* lat, const char* lng, bool locIsValid) {
    if (locIsValid) {
      JsonDocument JSONencode;
      JSONencode["variable"]="location";
      JSONencode["value"]="VentoSulDevice";
      JSONencode["location"]["type"]="Point";
      JSONencode["location"]["coordinates"][0]=lat;
      JSONencode["location"]["coordinates"][1]=lng;

      String JSONmessageBuffer;
      serializeJson(JSONencode,JSONmessageBuffer);
      char Buffer[MQTT_BUFFER_LENGTH];
      JSONmessageBuffer.toCharArray(Buffer,MQTT_BUFFER_LENGTH);
      client.publish(MQTT_TOPIC, Buffer);

    } 
    JsonDocument JSONencode;
    JSONencode["Voltage"] = voltage;
    JSONencode["Current"] = current;
    JSONencode["time"] = t;
  
    String JSONmessageBuffer;
    serializeJson(JSONencode,JSONmessageBuffer);
    // Serial.println("Publishing message");
    char Buffer[MQTT_BUFFER_LENGTH];
    JSONmessageBuffer.toCharArray(Buffer,MQTT_BUFFER_LENGTH);
    client.publish(MQTT_TOPIC, Buffer); 
}

void loop() {
    // put your main code here, to run repeatedly:
    if (!client.connected()) {
      reconnect();
    }
    if (!client.connected()) {
      delay(1000);
      Serial.println("000");
      updateTimeBuff();
      Serial.println(timeBuff);
      return;
    }
    
    unsigned long now = millis();
    if (now - lastMsg > 2000){
      int n;
      lastMsg = now;
      Serial.println("reading...");
      deserializeJson(doc, Serial2.readString());
      n = doc["n messages"];
      n = n - 1;
      // loop sending messages
      for (int i = 0; i < n; i++) {
        sendMQTT(doc[i]["Current"], doc[i]["Voltage"], doc[i]["time"], doc[i]["lat"], doc[i]["lng"], doc[i]["locIsValid"]);
      }
      updateTimeBuff();
      sendMQTT(doc[n]["Current"], doc[n]["Voltage"], timeBuff, doc[n]["lat"], doc[n]["lng"], doc[n]["locIsValid"]);
  }
}
