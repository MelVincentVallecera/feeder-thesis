#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Define pins for Sensors
#define motor_Pin 13
#define blower_Pin 12
#define echoUltra_Pin 11
#define trigUltra_Pin 10
#define ONE_WIRE_BUS 8
#define ammoniaDO_Pin 6
#define echoWater_Pin 3
#define trigWater_Pin 2
#define phPO_Pin A0
#define ammoniaAO_Pin A1
#define DO_Pin A2

#define TWO_POINT_CALIBRATION 0
#define VREF 5000      //VREF (mv)
#define ADC_RES 1024   //ADC Resolution
#define CAL1_V (131)   //mv
#define CAL1_T (25)    //℃
#define CAL2_V (1300)  //mv
#define CAL2_T (15)    //℃

#define RL 10
#define mo -0.263
#define bo 0.42
#define Ro 30

// Define feeding parameters
#define MAX_FEEDING 100  // Maximum amount of feed to give
#define MIN_FEEDING 20   // Minimum amount of feed to give
#define PH_TARGET 7.0    // Target PH value
#define TEMP_TARGET 25   // Target Water temperature
#define DO_TARGET 7.0    // Target Dissolved oxygen level
#define ACTIVITY_TARGET 10

#define ACTIVITY_TOLERANCE 5
#define PH_TOLERANCE 1.5
#define TEMP_TOLERANCE 2
#define DO_TOLERANCE 1.5

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
float phLevel;
float waterTemp;
String feedsLevel;
String fishActivity;
int interference_count = 0;

long duration, prevDuration;   // Variable to store the duration of sound wave travel
int distance;                  // Variable to store the distance measurement
int prev_distance = 0;         // Variable to store the previous distance measurement
unsigned long start_time = 0;  // Variable to store the start time of the 10-second period
String values;
int RXData = 0;

OneWire oneWire(ONE_WIRE_BUS);  // for watertemp
DallasTemperature sensors(&oneWire);

int16_t readDO(uint32_t voltage_mv, uint8_t temperature_c) {
#if TWO_POINT_CALIBRATION == 0
  uint16_t V_saturation = (uint32_t)CAL1_V + (uint32_t)35 * temperature_c - (uint32_t)CAL1_T * 35;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
#else
  uint16_t V_saturation = (int16_t)((int8_t)temperature_c - CAL2_T) * ((uint16_t)CAL1_V - CAL2_V) / ((uint8_t)CAL1_T - CAL2_T) + CAL2_V;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
#endif
}

void getSensorData() {  // trigger to get all sensor data
  fishActivity = getFishActivity(interference_count);
  waterTemp = getWaterTemp();
  feedsLevel = getFeedsLvl();
  dissolvedOxygen = getDO(waterTemp);
  ammoniaLevel = getAmmonia();
  phLevel = getPh();
}

String getFishActivity(int activity) {
  String activity_status;
  if(activity >= 0 && activity <= 3 ) {
    activity_status = "Least";
  }
  else if(activity >= 3 && activity <= 5) {
    activity_status = "Mid";
  }
  else if(activity >=5 && activity <= 10) {
    activity_status = "High";
  }
  return activity_status;
}
void checkActivity() {
  // Check if 10 seconds have passed since the start of the program
  if (millis() - start_time >= 10000) {
    // Reset the interference count and start time for the next 10-second period
    interference_count = 0;
    start_time = millis();
  }
  // Send a 10 microsecond pulse to the sensor to trigger a measurement
  digitalWrite(trigWater_Pin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigWater_Pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigWater_Pin, LOW);

  // Measure the duration of the sound wave travel
  duration = pulseIn(echoWater_Pin, HIGH);

  // Calculate the distance based on the duration and the speed of sound in air
  distance = duration * 0.034 / 2;

  // Check if the distance measurement is within 10 units of the previous measurement
  if (abs(duration - prevDuration) > 100) {
    // Increase the interference count
    interference_count++;
  }

  // Store the current distance measurement as the previous measurement for the next loop iteration
  prevDuration = duration;
  // Delay for 1 second before taking the next measurement
  delay(1000);
}

int getWaterLvl() {  // get water level
  float distance, duration;
  digitalWrite(trigWater_Pin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigWater_Pin, HIGH);
  delayMicroseconds(10);
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
  } else if (distance > 5 && distance < 15) {
    return status = ("3/4");
  } else if (distance > 15 && distance < 25) {
    return status = ("1/2");
  } else if (distance > 25 && distance < 32) {
    return status = ("1/4");
  } else {
    return status = ("Empty");
  }
}

String getDO(int temp) {
  String DO;
  Temperaturet = temp;
  ADC_Raw = analogRead(DO_Pin);
  delay(10);
  ADC_Voltage = uint32_t(VREF) * ADC_Raw / ADC_RES;
  DO = String(readDO(ADC_Voltage, Temperaturet) / 1000);
  return DO;
}

float getAmmonia() {
  float ammonia;
  float VRL;
  float Rs;
  float ratio;
  VRL = analogRead(ammoniaAO_Pin) * (5.0 / 1023.0);
  Rs = ((5.0 * RL) / VRL) - RL;
  ratio = Rs / Ro;
  ammonia = pow(10, ((log10(ratio) - bo) / mo));
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
  phValue = 2.0 * phValue;
  return phValue;
}

void feedNow() {
  digitalWrite(motor_Pin, LOW);   //turns on motor
  digitalWrite(blower_Pin, LOW);  //turns on blower
  delay(1000);
  digitalWrite(motor_Pin, HIGH);  //turns off motor
  delay(2000);                    //delay by 2s
  digitalWrite(blower_Pin, HIGH);
}

void run_Dispense(String feedsLevel, String dissolvedOxygen, float phLevel, float waterTemp) {  // Scheduled dispensing
  int feedAmount, duration, delayTime;
  float DO = dissolvedOxygen.toFloat();
  if (feedsLevel != "Empty") {                                                                                                                // Checks if container is empty
    if (abs(phLevel - PH_TARGET) <= PH_TOLERANCE && abs(waterTemp - TEMP_TARGET) <= TEMP_TOLERANCE && abs(DO - DO_TARGET) <= DO_TOLERANCE) {  // Checks if sensor data within target range
      feedAmount = calculate_feed_to_dispense(DO, phLevel, waterTemp);                                                                        // Calculates feed amount to be dispensed
      duration = calculate_feed_time(feedAmount);                                                                                             // Calculates how long the motor should run based on the amount of feed
      dispenseFeed(duration);                                                                                                                 // Dispense
    } else {                                                                                                                                  // If sensor data does not meet target range
      delayTime = map(abs(PH_TARGET - phLevel) + abs(TEMP_TARGET - waterTemp) + abs(DO_TARGET - DO), 0, 15, 0, 60);                           // Calculates how long it should wait before getting new sensor data
      delayTime = delayTime * 1000;
      delay(delayTime);                                                 // Delay based on the delay calculated by the function above
      getSensorData();                                                  // Gets new sensor data
      feedAmount = calculate_feed_to_dispense(DO, phLevel, waterTemp);  // Calculates feed amount to be dispensed based on new sensor data
      duration = calculate_feed_time(feedAmount);                       // Calculates how long the motor should run based on the amount of feed
      dispenseFeed(duration);                                           // Dispense
    }
  }
}

int calculate_feed_to_dispense(float DO, float ph, float temp) {
  int feed_amount = 0;
  feed_amount = map(abs(PH_TARGET - ph) + abs(TEMP_TARGET - temp) + abs(DO_TARGET - DO), 0, 15, MAX_FEEDING, MIN_FEEDING);
  return feed_amount;
}

int calculate_feed_time(int feed_amount) {
  int feed_drop_rate = 30;  // grams per second
  int time_needed = feed_amount / feed_drop_rate;
  time_needed = (time_needed + 1) * 1000;  // in seconds
  return time_needed;
}

void dispenseFeed(int duration) {
  digitalWrite(motor_Pin, LOW);   //turns on motor
  digitalWrite(blower_Pin, LOW);  //turns on blower
  delay(duration);
  digitalWrite(motor_Pin, HIGH);   //turns off motor
  delay(2000);                     //delay by 2s
  digitalWrite(blower_Pin, HIGH);  //turns off blower
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
  Serial.flush();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available() > 0) {
    RXData = Serial.read();
    switch (RXData) {
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
        run_Dispense(feedsLevel, dissolvedOxygen, phLevel, waterTemp);
        break;
    }
  }
  delay(3000);
  checkActivity();
  getSensorData();
  values = (dissolvedOxygen + ',' + waterTemp + ',' + fishActivity + ',' + feedsLevel + ',' + phLevel + ',' + ammoniaLevel + ',');
  Serial.println(values);
  Serial.flush();
}
