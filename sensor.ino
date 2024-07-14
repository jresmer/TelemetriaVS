#include <ArduinoJson.h>
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

#define RXp2 16
#define TXp2 17

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
  char time[30];
};
JsonDocument doc;
File msgQueue;
String serialStr;
int queueSize = 0;
int lastSentMessage = 0;
message msg;
message copyto;
byte *buff;

void setup() {

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  Serial.begin(9600);
  Serial2.begin(9600);
  lcd.init();
  lcd.setBacklight(HIGH);
  // writes the value labels on display
  lcd.setCursor(0,0);
  lcd.print("Tens Corr Vel");
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
/////////////////////////////////////////////////////////////////
/* Reads all sensor values and attributes them to the buffer b */
/* b's is a parameter passed through reference to the function */
/////////////////////////////////////////////////////////////////
void updateReads() {

  updateVoltage();
  updateCurrent();
  updateTemp();
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
      doc[i]["time"] = buffer[i].time;
    }
    doc[queueSize]["Current"] = current;
    doc[queueSize]["Voltage"] = batteryVoltage;
    serializeJson(doc, Serial2);
    msgQueue.close();
  } else {
    JsonDocument d;
    doc[0]["Current"] = current;
    doc[0]["Voltage"] = batteryVoltage;
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
}
