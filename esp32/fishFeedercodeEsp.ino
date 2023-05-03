#include <Arduino.h>
#include <ESP8266WiFi.h>

#include <FirebaseESP32.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <string.h>

#define WIFI_SSID "Balais Globe"
#define WIFI_PASSWORD "balaisglobe123"
#define FIREBASE_HOST "fish-feed-fe756-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "AIzaSyAkJnRiEeAwfSSLv7-2xdfUhTTKFrlVoXY"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 28800, 19800);

FirebaseData timer, feed, sensor;
FirebaseConfig config;

String stimer;
String Str[] = { "00:00", "00:00", "00:00", "00:00", "00:00", "00:00", "00:00", "00:00", "00:00", "00:00" };
String values, sensor_data;
int i, feednow = 0;

void checkFeednow() {
  Firebase.getInt(feed, "feednow");
  feednow = feed.to<int>();

  if (feednow == 1) {     // Direct Feeding
    Serial.println("1");  // send command to arduino
    feednow = 0;
    Firebase.setInt(feed, "/feednow", feednow);
  }
}

void checkSchedules() {
  int n;
  Firebase.getInt(feed, "count");
  n = feed.to<int>();
  timeClient.update();

  for (i = 0; i < n; i++) {
    String path = "timers/timer" + String(i) + "/time";
    Firebase.getString(timer, path);
    stimer = timer.to<String>();
    Str[i] = stimer.substring(0, 5);
  }
  String currentTime = String(timeClient.getHours()) + ":" + String(timeClient.getMinutes());
  for (int j = 0; j < n; j++) {
    if (Str[j] == currentTime) {
      Serial.println("2");
      delay(50000);
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  timeClient.begin();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
}

void loop() {
  // put your main code here, to run repeatedly:
  timeClient.update();
  checkFeednow();
  checkSchedules();
  bool Sr = false;
  while (Serial.available() > 0) {
    sensor_data = Serial.readString();
    Sr = true;
  }
  values = sensor_data;
  if (Sr == true) {
    //get comma indexes from values variable
    int firstCommaIndex = values.indexOf(',');
    int secondCommaIndex = values.indexOf(',', firstCommaIndex + 1);
    int thirdCommaIndex = values.indexOf(',', secondCommaIndex + 1);
    int fourthCommaIndex = values.indexOf(',', thirdCommaIndex + 1);
    int fifthCommaIndex = values.indexOf(',', fourthCommaIndex + 1);
    int sixthCommaIndex = values.indexOf(',', fifthCommaIndex + 1);

    //get sensors data from values variable by  spliting by commas and put in to variables
    String dissolvedOxygen = values.substring(0, firstCommaIndex);
    String waterTemp = values.substring(firstCommaIndex + 1, secondCommaIndex);
    String waterLevel = values.substring(secondCommaIndex + 1, thirdCommaIndex);
    String feeds = values.substring(thirdCommaIndex + 1, fourthCommaIndex);
    String ph = values.substring(fourthCommaIndex + 1, fifthCommaIndex);
    String ammonia = values.substring(fifthCommaIndex + 1, sixthCommaIndex);

    //store inside data as string in firebase
    Firebase.setString(sensor, "/sensors/dissolvedOxygen", dissolvedOxygen);
    Firebase.setString(sensor, "/sensors/waterTemp", waterTemp);
    Firebase.setString(sensor, "/sensors/waterLevel", waterLevel);
    Firebase.setString(sensor, "/sensors/feeds", feeds);
    Firebase.setString(sensor, "/sensors/ph", ph);
    Firebase.setString(sensor, "/sensors/ammonia", ammonia);
  }
}
