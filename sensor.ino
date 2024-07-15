#include <ArduinoJson.h>
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TinyGPSPlus.h>

#define RXp2 16
#define TXp2 17
#define RXp3 14
#define TXp3 15
////////////////////////////////////////////////////////
/* Variables and Consts for current and voltage reads */
////////////////////////////////////////////////////////
int analogValue;
float shuntVoltage, batteryVoltage, current, temperature, spd;
const float c0 = 1 / (40.0 * 0.1);
const float c1 = 5 / 1024;
const float R = 3;
//////////////////////////////////
/* Display management variables */
//////////////////////////////////
LiquidCrystal_I2C lcd(0x27,16,4);
char values[16];
String s, s0;
char char0[5];
float* addresses[3] = {&batteryVoltage, &current, &spd};
////////////////////////////////
/* Message struct declaration */
////////////////////////////////
struct message 
{
  float voltage, current, temperature;
  bool locationIsValid;
  char lat[14];
  char lng[14];
  char time[30];
};
////////////////////////////////////////////////////
/* Json and SD libs variables and aux declaration */
////////////////////////////////////////////////////
JsonDocument doc;
File msgQueue;
String serialStr;
int queueSize = 0;
int lastSentMessage = 0;
message msg;
message copyto;
byte *buff;
////////////////////////////////////////////////////
/* GPS headers and and data variables declaration */
////////////////////////////////////////////////////
const uint32_t GPS_BAUD = 9600; // Default baud of NEO-6M is 9600
const uint32_t GPS_DELAY = 1000;
double llat, llng;
char lat[14], lng[14];
time_t gpsNow;
TinyGPSPlus gps; // The tinyGPS object

void setup() {

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  Serial.begin(9600);
  Serial2.begin(9600);
  Serial3.begin(GPS_BAUD);
  lcd.init();
  lcd.setBacklight(HIGH);
  // writes the value labels on display
  lcd.setCursor(0,0);
  lcd.print("Tens Corr Velc");
  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
}

void updateTemp() {
  // TODO
  temperature = 42.3 + rand() % 10;
}

void updateCurrent() {

  analogValue = analogRead(A0);
  shuntVoltage = analogValue * c1;
  current = shuntVoltage * c0;
}

void updateVoltage() {

  analogValue = analogRead(A1);
  float voltageDivider = analogValue * c1;
  batteryVoltage = voltageDivider * R;
}

void updateGPSRead() {

  // TODO - double check Serial3.read() - might have to read each byte
  while (Serial3.available()) {
    if (gps.encode(Serial3.read())) {
      snprintf(lat, 14, "%.10f",gps.location.lat());
      snprintf(lng, 14, "%.10f",gps.location.lng());
      spd = gps.speed.kmph();
      if (gps.speed.isValid())
        spd = -1;
    }
  } 
}
/////////////////////////////////////////////////////////////////
/* Reads all sensor values and attributes them to the buffer b */
/* b's is a parameter passed through reference to the function */
/////////////////////////////////////////////////////////////////
void updateReads() {

  updateVoltage();
  updateCurrent();
  updateTemp();
  updateGPSRead();
}

void formatDisplayData() {
  // formating datas
  s = "";
  for (int i = 0; i < 3; i++) {
    s0 = *(addresses[i]);
    s0.toCharArray(char0, 5);
    String s1(s0);
    s = s + " " + s1;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  updateReads();
  formatDisplayData();
  lcd.setCursor(0,1);
  lcd.print(s);
  if (queueSize) {
    // reads queued messages into buffer
    int msgSize = sizeof(message);
    int flag;
    message buffer[queueSize];
    msgQueue = SD.open("q.txt", FILE_READ);
    flag = msgQueue.seek(lastSentMessage * msgSize);
    if (!flag) {
      Serial.println("Failed to seek ");
      return;
    }
    msgQueue.read(buffer, queueSize * msgSize);
    doc["n messages"] = queueSize + 1;
    // TODO - treat data and serialize to ESP32
    for (int i = 0; i < queueSize; i++) {
      doc[i]["Current"] = buffer[i].current;
      doc[i]["Voltage"] = buffer[i].voltage;
      doc[i]["Temperature"] = buffer[i].temperature;
      doc[queueSize]["locIsValid"] = buffer[i].locationIsValid;
      doc[i]["lat"] = buffer[i].lat;
      doc[i]["lng"] = buffer[i].lng;
      doc[i]["time"] = buffer[i].time;
    }
    doc[queueSize]["Current"] = current;
    doc[queueSize]["Voltage"] = batteryVoltage;
    doc[queueSize]["Temperature"] = temperature;
    doc[queueSize]["lat"] = lat;
    doc[queueSize]["lng"] = lng;
    doc[queueSize]["locIsValid"] = gps.location.isValid();
    msgQueue.close();
    serializeJson(doc, Serial2);
  } else {
    JsonDocument d;
    doc[0]["Current"] = current;
    doc[0]["Voltage"] = batteryVoltage;
    doc[0]["Temperature"] = temperature;
    doc[0]["lat"] = lat;
    doc[0]["lng"] = lng;
    doc[0]["locIsValid"] = gps.location.isValid();
    doc["n messages"] = 1;
    serializeJson(doc, Serial2);
  }
  serialStr = Serial2.readString();
  // adding message to log
  if (serialStr == "000") {
    int flag;
    serialStr = Serial2.readString();
    msg.current = current;
    msg.voltage = batteryVoltage;
    msg.temperature = temperature;
    snprintf(msg.lat, 14, lat);
    snprintf(msg.lng, 14, lng);
    msg.locationIsValid = gps.location.isValid();
    serialStr.toCharArray(msg.time, 30);
    // acces msg as bytes
    buff = (byte *) &msg;
    msgQueue = SD.open("q.txt", FILE_WRITE);
    // if file fails to open continue
    if (!msgQueue)
      return;
    flag = msgQueue.seek((lastSentMessage + queueSize) * sizeof(message));
    if (!flag) {
      Serial.println("Failed to seek ");
      return;
    }
    msgQueue.write(buff, sizeof(message));
    queueSize++;
    msgQueue.close();
  } else {
    lastSentMessage = queueSize;
    queueSize = 0;    
  }
  delay(1000);
}
