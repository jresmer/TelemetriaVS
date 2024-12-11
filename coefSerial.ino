////////////////////////////////////////////////////////
/* Variables and Consts for current and voltage reads */
////////////////////////////////////////////////////////
int analogValue;
//const int R1 = 56000; // resistor 5x maior
//const int R2 = 10000; // resistor de verificação de tensão 
const int gain = 1000; // ganho do ampop
float shuntVoltage, batteryVoltage, current;
const float due = 3.3 / 4095.0; 
const float resiShunt = 0.0001; // resistênica do shunt
const float c1 = 4.0217391304; // fator de correção da tensão 

void setup() {

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  Serial.begin(9600);
  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  File coefCSV = SD.open("coef0.csv", FILE_WRITE);
  coefCSV.println("Corrente,");
  coefCSV.close()
}

void updateCurrent() {

  analogValue = analogRead(A0);
  shuntVoltage = analogValue * due;
  current = (shuntVoltage/gain)/resiShunt;
}

void updateVoltage() {

  analogValue = analogRead(A1);
  //float voltageDivider = (R1+R2)/R2 ;
  batteryVoltage = analogValue * due * c1 * 6.6;
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

  // write in SD
  String csvRow;
  csvRow = current;
  Serial.println(csvRow);

  delay(1000);
}
