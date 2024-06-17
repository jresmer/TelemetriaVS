#include <ArduinoJson.h>
#include <Arduino.h>

////////////////////////////////////////////////////////
/* Variables and Consts for current and voltage reads */
////////////////////////////////////////////////////////
int analogValue;
float shuntVoltage, batteryVoltage, current;
const float c0 = 1 / (40.0 * 0.1);
const float c1 = 5 / 1023;
const float R = 3;

long int buffer[4];
JsonDocument doc;
#define RXp2 16
#define TXp2 17

void setup() {

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  Serial.begin(9600);
  Serial2.begin(9600);
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
}

void loop() {
  // put your main code here, to run repeatedly:
  updateReads();
  doc["Current"] = current;
  doc["Voltage"] = batteryVoltage;
  serializeJson(doc, Serial2);
  Serial2.readString();
  delay(1000);
}
