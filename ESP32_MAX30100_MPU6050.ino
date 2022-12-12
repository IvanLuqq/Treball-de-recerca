//Firebase
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include "time.h"

//I2C
#include <Wire.h>  
                                                                                                            
//MAX30100
#include "MAX30100_PulseOximeter.h"

//MPU6050
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>



//Firebase
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define API_KEY ""
#define DATABASE_URL "" 
#define USER_EMAIL ""
#define USER_PASSWORD ""

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
unsigned long count = 0;
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 16200;
const int   daylightOffset_sec = 3600;

//MAX30100
#define REPORTING_PERIOD_MS 100
PulseOximeter pox;
uint32_t tsLastReport = 0;
void onBeatDetected()
{
Serial.println("Beat!");
}

//MPU6050
Adafruit_MPU6050 mpu;

void setup()
{
Serial.begin(115200);
Wire.begin();

//Firebase
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Failed connection");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Firebase.setDoubleDigits(5);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
 

//MPU6050
 while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit MPU6050 test!");

  // Try to initialize!
  if (!mpu.begin(0x68)) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  Serial.println("");
  delay(100);

   //MAX30100
  if (!pox.begin()) {
  Serial.println("FAILED");
  for(;;);
  } else {
  Serial.println("SUCCESS");
  }
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);

}

void loop()
{
//Time
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  char timeWeekDay[100];
  strftime(timeWeekDay,100, "%d %B %Y, %H:%M:%S", &timeinfo);



//MAX30100
if (pox.begin()){
  pox.update();
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
  Serial.print("Heart rate:");
  Serial.print(pox.getHeartRate());
  Serial.print("bpm / SpO2:");
  Serial.print(pox.getSpO2());
  Serial.println("%");
  tsLastReport = millis();
}}

//mpu.begin(0x69);

 

//MPU6050
if (mpu.begin(0x68)){

sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  Serial.print("Acceleration X: ");
  Serial.print(a.acceleration.x);
  Serial.print(", Y: ");
  Serial.print(a.acceleration.y);
  Serial.print(", Z: ");
  Serial.print(a.acceleration.z);
  Serial.println(" m/s^2");
  
  Serial.print("Rotation X: ");
  Serial.print(g.gyro.x);
  Serial.print(", Y: ");
  Serial.print(g.gyro.y);
  Serial.print(", Z: ");
  Serial.print(g.gyro.z);
  Serial.println(" rad/s");
  
  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println(" degC");
  Serial.println("");

  FirebaseJson json;

  json.add(String(timeWeekDay), a.acceleration.x);
  Firebase.updateNode(fbdo, F("/new/json/acceleration/x"), json) ? "ok" : fbdo.errorReason().c_str();

  json.add(String(timeWeekDay), a.acceleration.y);
  Firebase.updateNode(fbdo, F("/new/json/acceleration/y"), json) ? "ok" : fbdo.errorReason().c_str();

  json.add(String(timeWeekDay), a.acceleration.z);
  Firebase.updateNode(fbdo, F("/new/json/acceleration/z"), json) ? "ok" : fbdo.errorReason().c_str();

  json.add(String(timeWeekDay), g.gyro.x);
  Firebase.updateNode(fbdo, F("/new/json/rotation/x"), json) ? "ok" : fbdo.errorReason().c_str();

  json.add(String(timeWeekDay), g.gyro.y);
  Firebase.updateNode(fbdo, F("/new/json/rotation/y"), json) ? "ok" : fbdo.errorReason().c_str();
  
  json.add(String(timeWeekDay), g.gyro.z);
  Firebase.updateNode(fbdo, F("/new/json/rotation/z"), json) ? "ok" : fbdo.errorReason().c_str();

  json.add(String(timeWeekDay), temp.temperature);
  Firebase.updateNode(fbdo, F("/new/json/temperature"), json) ? "ok" : fbdo.errorReason().c_str();

 }
 // mpu.begin(0x61);
  
  delay(5000);
//Wire.beginTransmission(0x00);
}
