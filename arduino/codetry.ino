#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 14
#define motor_Pin 13
#define blower_Pin 12
#define echoUltra_Pin 11
#define trigUltra_Pin 10
#define DO_Pin 9
#define IR_Pin 8
#define ammoniaAO_Pin 7
#define ammoniaDO_Pin 6
#define phTO_Pin 5
#define phPO_Pin A0
#define echoWater_Pin 3
#define trigWater_Pin 2

#define TWO_POINT_CALIBRATION 0
#define READ_TEMP (29)
#define VREF 5000      //VREF (mv)
#define ADC_RES 1024   //ADC Resolution
#define CAL1_V (131)   //mv
#define CAL1_T (25)    //℃
#define CAL2_V (1300)  //mv
#define CAL2_T (15)    //℃

#define DO_SA

const uint16_t DO_Table[41] = {
  14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
  11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
  9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
  7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410
};

uint8_t Temperaturet;
uint16_t ADC_Raw;
uint16_t ADC_Voltage;
uint16_t DO;

unsigned long int avgValue;
float b;
int buf[10], temp;

String dissolvedOxygen;
float ammoniaLevel;
int waterLevel;
float phLevel;
float waterTemp;
String feedsLevel;

String values;
int RXData = 0;

OneWire oneWire(ONE_WIRE_BUS);  // for watertemp
DallasTemperature sensors(&oneWire);

int16_t readDO(uint32_t voltage_mv, uint8_t temperature_c) {
#if TWO_POINT_CALIBRATION == 00
  uint16_t V_saturation = (uint32_t)CAL1_V + (uint32_t)35 * temperature_c - (uint32_t)CAL1_T * 35;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
#else
  uint16_t V_saturation = (int16_t)((int8_t)temperature_c - CAL2_T) * ((uint16_t)CAL1_V - CAL2_V) / ((uint8_t)CAL1_T - CAL2_T) + CAL2_V;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
#endif
}

void getSensorData() {  // trigger to get all sensor data
  dissolvedOxygen = getDO();
  ammoniaLevel = getAmmonia();
  phLevel = getPh();
  waterLevel = getWaterLvl();
  waterTemp = getWaterTemp();
  feedsLevel = getFeedsLvl();
}

int getWaterLvl() {  // get water level
  float distance, duration;
  digitalWrite(trigWater_Pin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigWater_Pin, HIGH);
  delayMicroseconds(20);
  digitalWrite(trigWater_Pin, LOW);
  duration = pulseIn(echoWater_Pin, HIGH);
  distance = duration * 0.034 / 2;

  return distance;
}

float getWaterTemp() {
  float temperature;
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);
  return temperature;
}

String getFeedsLvl() {
  String status;
  long duration;
  int distance;

  digitalWrite(trigUltra_Pin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigUltra_Pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigUltra_Pin, LOW);
  duration = pulseIn(echoUltra_Pin, HIGH);
  distance = duration * 0.034 / 2;
  if (distance < 5) {
    return status = ("Full");
  } else if (distance < 10 && distance > 5) {
    return status = ("Half");
  } else if (distance < 15 && distance > 10) {
    return status = ("Low");
  } else {
    return status = ("Empty");
  }
}

String getDO() {
  String DO;
  Temperaturet = (uint8_t)READ_TEMP;
  ADC_Raw = analogRead(DO_Pin);
  delay(10);
  ADC_Voltage = uint32_t(VREF) * ADC_Raw / ADC_RES;
  DO = String(readDO(ADC_Voltage, Temperaturet) / 1000);
  return DO;
}

float getAmmonia() {
  float ammonia;
  return ammonia;
}

float getPh() {
  for (int i = 0; i < 10; i++)  //Get 10 sample value from the sensor for smooth the value
  {
    buf[i] = analogRead(phPO_Pin);
    delay(10);
  }
  for (int i = 0; i < 9; i++)  //sort the analog from small to large
  {
    for (int j = i + 1; j < 10; j++) {
      if (buf[i] > buf[j]) {
        temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }
  avgValue = 0;
  for (int i = 2; i < 8; i++)  //take the average value of 6 center sample
    avgValue += buf[i];
  float phValue = (float)avgValue * 5.0 / 1024 / 6;  //convert the analog into millivolt
  phValue = 3.5 * phValue;
  return phValue;
}

void run_Dispense(String feedsLevel, String dissolvedOxygen, float phLevel, float ammoniaLevel, int waterLevel, float waterTemp) {  //controls dispensing
  int feedAmount = calculate_feed_dispense(feedsLevel, dissolvedOxygen, phLevel, ammoniaLevel, waterLevel, waterTemp);
  int duration = calculate_feed_time(feedAmount);
  digitalWrite(motor_Pin, LOW);   //turns on motor
  digitalWrite(blower_Pin, LOW);  //turns on blower
  delay(duration);
  digitalWrite(motor_Pin, HIGH);   //turns off motor
  delay(2000);                     //delay by 2s
  digitalWrite(blower_Pin, HIGH);  //turns off blower
}

void feedNow() {
  digitalWrite(motor_Pin, LOW);   //turns on motor
  digitalWrite(blower_Pin, LOW);  //turns on blower
  delay(2000);
  digitalWrite(motor_Pin, HIGH);  //turns off motor
  delay(2000);                    //delay by 2s
  digitalWrite(blower_Pin, HIGH);
}

int calculate_feed_dispense(String feedsLevel, String dissolvedOxygen, float phLevel, float ammoniaLevel, int waterLevel, float waterTemp) {
  int feed_amount = 0;
  DO = dissolvedOxygen.toInt();
  if (feedsLevel != 'Empty') {
    if (DO > 6 && phLevel > 5 && phLevel < 8 && waterTemp > 20) {
      feed_amount = 300;
    }
    if (DO > 6 && phLevel > 4 && phLevel < 9 && waterTemp > 20) {
      feed_amount = 200;
    } else {
      feed_amount = 100;
    }
  }
  return feed_amount;
}

int calculate_feed_time(int feed_amount) {
  int feed_drop_rate = 100;  // grams per second
  int time_needed = feed_amount / feed_drop_rate;
  time_needed = (time_needed + 1) * 1000;  // in seconds
  return time_needed;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(trigWater_Pin, OUTPUT);  // for water lvl
  pinMode(echoWater_Pin, INPUT_PULLUP);

  pinMode(motor_Pin, OUTPUT);   // for motor control
  pinMode(blower_Pin, OUTPUT);  // for blower control

  pinMode(trigUltra_Pin, OUTPUT);  // ultrasonic for feeds
  pinMode(echoUltra_Pin, INPUT);
  
  digitalWrite(motor_Pin, HIGH);  //turns off motor               //delay by 2s
  digitalWrite(blower_Pin, HIGH);
  sensors.begin();  // temperature sensor
}

void loop() {
  // put your main code here, to run repeatedly:
  getSensorData();
  values = (dissolvedOxygen + ',' + waterTemp + ',' + waterLevel + ',' + feedsLevel + ',' + phLevel + ',' + ammoniaLevel);
  Serial.flush();
  delay(1000);
  Serial.println(values);

  if (Serial.available() > 0) {
    RXData = Serial.read();
    switch(RXData) {
    case '1':
      Serial.flush();
      getSensorData();
      delay(2000);
      feedNow();
      break;
    case '2':
      Serial.flush();
      getSensorData();
      delay(2000);
      run_Dispense(feedsLevel, dissolvedOxygen, phLevel, ammoniaLevel, waterLevel, waterTemp);
      break;
    }
  }
delay(2000);
}
