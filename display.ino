#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//Inicializa o display no endereco 0x27
LiquidCrystal_I2C lcd(0x27,16,2);
char values[16];
String s, s0;
char char0[5];
float temp, tens, corr, vel;
float* addresses[4] = {&temp, &tens, &corr, &vel};
 
void setup() {
  // put your setup code here, to run once:
  lcd.init();
  Serial.begin(9600);
}

void updateValues() {
  temp = (rand() % 100) / 100 + rand() % 90;
  tens = (rand() % 100) / 100 + rand() % 70;
  corr = (rand() % 100) / 100 + rand() % 30;
  vel = (rand() % 100) / 100 + rand() % 40;
}

void loop() {
  // put your main code here, to run repeatedly:
  lcd.setBacklight(HIGH);
  lcd.setCursor(0,0);
  lcd.print("Tp Ts Cr Vl");
  lcd.setCursor(0,1);
  updateValues();
  // formating datas
  s = "";
  for (int i = 0; i < 4; i++) {
    s0 = *(addresses[i]);
    s0.toCharArray(char0, 5);
    String s1(s0);
    s += s + " " + s1;
  }
  // snprintf(values, 16, "%d %d %d %d", temp, tens, corr, vel);
  Serial.println(s);
  lcd.print(s);
  delay(1000);
}
