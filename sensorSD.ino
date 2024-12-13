#include <ArduinoJson.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

/////////////////////
/* Pin declaration */
/////////////////////
const int currentPin = A0;
const int voltagePin = A1;
const int temperaturePin = A2;
const int RX_PIN, TX_PIN;
SoftwareSerial GPSSerial(RX_PIN, TX_PIN);
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
String s;
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
TinyGPSPlus gps; // The tinyGPS object

void setup() {

  pinMode(currentPin, INPUT);
  pinMode(voltagePin, INPUT);
  pinMode(temperaturePin, INPUT);
  Serial.begin(9600);
  GPSSerial.begin(GPS_BAUD);
  lcd.init();
  lcd.setBacklight(HIGH);
  // writes the value labels on display
  lcd.setCursor(0,0);
  lcd.print("Tens Corr Velc");
  if (!SD.begin(4)) {
    while (1);
  }
}

void updateTemp() {

  int readValue = analogRead(temperaturePin);
  readValue = readValue * 488;
  temperature = readValue / 1000;
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
  while (GPSSerial.available()) {
    if (gps.encode(GPSSerial.read())) {
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
  String s0;
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
    File msgQueue = SD.open("q.txt", FILE_READ);
    flag = msgQueue.seek((lastSentMessage + 1) * msgSize);
    if (!flag) {
      return;
    }
    doc["n messages"] = queueSize + 1;
    // TODO - treat data and serialize to ESP32
    for (int i = 0; i < queueSize; i++) {
      msgQueue.read((byte *) &copyto, queueSize * msgSize);
      doc[i+1]["Current"] = copyto.current;
      doc[i+1]["Voltage"] = copyto.voltage;
      doc[i+1]["Temperature"] = copyto.temperature;
      doc[i+1]["locIsValid"] = copyto.locationIsValid;
      doc[i+1]["lat"] = copyto.lat;
      doc[i+1]["lng"] = copyto.lng;
      doc[i+1]["time"] = copyto.time;
    }
    doc[queueSize]["Current"] = current;
    doc[queueSize]["Voltage"] = batteryVoltage;
    doc[queueSize]["Temperature"] = temperature;
    doc[queueSize]["lat"] = lat;
    doc[queueSize]["lng"] = lng;
    doc[queueSize]["locIsValid"] = gps.location.isValid();
    msgQueue.close();
    serializeJson(doc, Serial);
  } else {
    doc[0]["Current"] = current;
    doc[0]["Voltage"] = batteryVoltage;
    doc[0]["Temperature"] = temperature;
    doc[0]["lat"] = lat;
    doc[0]["lng"] = lng;
    doc[0]["locIsValid"] = gps.location.isValid();
    doc["n messages"] = 1;
    serializeJson(doc, Serial);
  }
  String serialStr = Serial.readString();
  // adding message to log
  if (serialStr == "000") {
    int flag;
    serialStr = Serial.readString();
    msg.current = current;
    msg.voltage = batteryVoltage;
    msg.temperature = temperature;
    snprintf(msg.lat, 14, lat);
    snprintf(msg.lng, 14, lng);
    msg.locationIsValid = gps.location.isValid();
    serialStr.toCharArray(msg.time, 30);
    // acces msg as bytes
    buff = (byte *) &msg;
    File msgQueue = SD.open("q.txt", FILE_WRITE);
    // if file fails to open continue
    if (!msgQueue)
      return;
    flag = msgQueue.seek((lastSentMessage + queueSize) * sizeof(message));
    if (!flag) {
      return;
    }
    msgQueue.write(buff, sizeof(message));
    queueSize++;
    msgQueue.close();
  } else {
    lastSentMessage = lastSentMessage + queueSize;
    queueSize = 0;    
  }
  delay(2000);
}
