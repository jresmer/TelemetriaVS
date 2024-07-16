#include <ArduinoJson.h>
#include <SPI.h>
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
////////////////////////////////////////////
/* Json lib variables and aux declaration */
////////////////////////////////////////////
JsonDocument doc;
short int qStart = 0;
short int qEnd = -1;
bool lastMsgSuccess = true;
message msgQueue[12];
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
}

void updateTemp() {
  
  int readValue = analogRead(temperaturePin);
  readValue = readValue * 488;
  temperature = readValue / 1000;
}

void updateCurrent() {

  analogValue = analogRead(currentPin);
  shuntVoltage = analogValue * c1;
  current = shuntVoltage * c0;
}

void updateVoltage() {

  analogValue = analogRead(voltagePin);
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
  int n_msgs = 0;
  if (!lastMsgSuccess) {
    short int end = (qEnd + 1) % 12;
    do {
      doc[n_msgs]["Current"] = msgQueue[qStart].current;
      doc[n_msgs]["Voltage"] = msgQueue[qStart].voltage;
      doc[n_msgs]["Temperature"] = msgQueue[qStart].temperature;
      doc[n_msgs]["lat"] = msgQueue[qStart].lat;
      doc[n_msgs]["lng"] = msgQueue[qStart].lng;
      doc[n_msgs]["locIsValid"] = msgQueue[qStart].locationIsValid;
      doc[n_msgs]["time"] = msgQueue[qStart].time;
      n_msgs++;
      qStart = (qStart + 1) % 12;
    } while (qStart != end);
  }
  doc[n_msgs-1]["Current"] = current;
  doc[n_msgs-1]["Voltage"] = batteryVoltage;
  doc[n_msgs-1]["Temperature"] = temperature;
  doc[n_msgs-1]["lat"] = lat;
  doc[n_msgs-1]["lng"] = lng;
  doc[n_msgs-1]["locIsValid"] = gps.location.isValid();
  doc["n messages"] = n_msgs + 1;
  serializeJson(doc, Serial);
  lastMsgSuccess = true;
  String serialStr = Serial.readString();
  // adding message to log
  if (serialStr == "000") {
    lastMsgSuccess = false;
    qEnd = (qEnd + 1) % 12;
    if (qEnd == qStart)
      qStart = (qStart + 1) % 12;
    serialStr = Serial.readString();
    msgQueue[qEnd].current = current;
    msgQueue[qEnd].voltage = batteryVoltage;
    msgQueue[qEnd].temperature = temperature;
    snprintf(msgQueue[qEnd].lat, 14, lat);
    snprintf(msgQueue[qEnd].lng, 14, lng);
    msgQueue[qEnd].locationIsValid = gps.location.isValid();
    serialStr.toCharArray(msgQueue[qEnd].time, 30);
  } 
  delay(1000);
}
