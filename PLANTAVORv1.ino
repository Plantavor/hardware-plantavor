#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include "time.h"
#include "sntp.h"
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <DHT.h>

DHT dht(26, DHT11);

#define adcPin 34      // pin input sensor pH tanah
#define soil_pin 35    // pin input sensor soil moisture
#define npk_pin 32     // pin input sensor npk
#define ldr_pin 33     // pin input sensor ldr

int ldr_val = 0;
int soil = 0;
int ADC;
int NPK;
float nilaipH;

const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long  gmtOffset_sec = 25200;

#define WIFI_SSID "Plantavor"
#define WIFI_PASSWORD "Girlsuperpower"
#define API_KEY "AIzaSyDVeFQQocG09ELp1DxWbhTKOCfqv8UxXJw"
#define DATABASE_URL "https://plantavor-2-default-rtdb.asia-southeast1.firebasedatabase.app/" 
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

String timeString;
String dateString;


void setup()
{
  Serial.begin(115200);
  dht.begin();  
  configTime(gmtOffset_sec, 0, ntpServer1, ntpServer2);
  analogReadResolution(10); 
  // pinMode(DMSpin, OUTPUT);
  // pinMode(indikator, OUTPUT);
  // digitalWrite(DMSpin,HIGH);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();


  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

}

void loop(){
    //TIME
 struct tm timeinfo;
  // if (getLocalTime(&timeinfo)) {
  //   String timeString = String(timeinfo.tm_hour < 10 ? ("0" + String(timeinfo.tm_hour)) : String(timeinfo.tm_hour)) + ":";
  //   timeString += (timeinfo.tm_min < 10 ? ("0" + String(timeinfo.tm_min)) : String(timeinfo.tm_min)) + ":";
  //   timeString += (timeinfo.tm_sec < 10 ? ("0" + String(timeinfo.tm_sec)) : String(timeinfo.tm_sec));
  //   Serial.println("------------------------");
  //   Serial.println(timeString);

  //   String dateString;
  //   char buffer[9];
  //   sprintf(buffer, "%02d%02d%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
  //   dateString = buffer;
  //   Serial.println(dateString);
  // } else {
  //   Serial.println("No time available (yet)");
  // }

    //LDR SENSOR
  ldr_val = analogRead(ldr_pin);
  Serial.println("------------------------");
  Serial.print("LDR : ");
  Serial.println(ldr_val);

    //DHT11
  float temp_val = dht.readTemperature();
  float hum_val = dht.readHumidity();
  Serial.println("----------");
  Serial.print("Temp: ");
  Serial.print(temp_val);
  Serial.println("°C ");
  Serial.print("Humidity: ");
  Serial.print(hum_val);
  Serial.println(" % ");

    //SOIL MOISTURE
  soil= analogRead(soil_pin);
  int anl_s = map(soil, 0, 4095, 0, 1023);
  int soil_val = map(anl_s, 0, 1023, 100, 20);

  Serial.println("----------");
  Serial.print("Soil : ");
  Serial.print(soil_val);
  Serial.println(" % ");
  Serial.println("------------------------");



    //pH SENSOR
  // digitalWrite(DMSpin,LOW);      // aktifkan DMS
  // digitalWrite(indikator, HIGH); // led indikator built-in ESP32 menyala
  // delay(10*1000);
  ADC = analogRead(adcPin);
  // int anxpH = map(ADC, 0, 4095, 0, 1023);
  // float anpH1 = map(anxpH, 0, 950, 10, 35);

  // pH  = (-0.0333*ADC) + 9.8340;  // ini adalah rumus regresi linier yang wajib anda ganti!
  // if (pH != lastReading) { 
  //   lastReading = pH; 
  // }
  //nilaipH = (0.0476 * ADC) - 5.983;
  // nilaipH = (-0.0139 * ADC) + 7.7851;
  nilaipH = (-0.0009 * ADC) + 7.2341;
  if (nilaipH < 1) {
    nilaipH = 1;
  }
  else if ( nilaipH > 14)
    nilaipH = 14;
  else {
    nilaipH = nilaipH;
  }
  
  Serial.print("ADC pH Sensor : ");
  Serial.println(ADC);
  Serial.print("pH : ");
  Serial.println(nilaipH);
  Serial.println("------------------------");

  // digitalWrite(DMSpin,HIGH);
  // digitalWrite(indikator,LOW);

  //NPK SENSOR

  NPK = analogRead(npk_pin);
  int val = map(NPK, 0, 4095, 0, 1023);

  //Rumus1
  int Nx = map(val, 0, 1023, 412, 60);
  if (Nx < 0) {
    Nx = 0;
  }
  else if ( Nx > 900)
    Nx = 900;
  else {
    Nx = Nx;
  }

  int Px = map(val, 100, 1023, 39, 5);
  if (Px < 0) {
    Px = 0;
  }
  else if ( Px > 100)
    Px = 100;
  else {
    Px = Px;
  }

  int Kx = map(val, 30, 1023, 270, 60);
  if (Kx < 0) {
    Kx = 0;
  }
  else if ( Kx > 700)
    Kx = 700;
  else {
    Kx = Kx;
  }
  
  //Rumus2
  // int UN = (-0.1865*NPK)+820.9;
  // int UP = (-0.0201*NPK)+29.423;
  // int UK = (-0.1717*NPK)+231.55;
  
  Serial.println("------------------------");

  Serial.print("N : ");
  Serial.println(Nx);
  Serial.print("P : ");
  Serial.println(Px);
  Serial.print("K : ");
  Serial.println(Kx);

  Serial.println("------------------------");

  // Serial.print("N : ");
  // Serial.println(UN);
  // Serial.print("P : ");
  // Serial.println(UP);
  // Serial.print("K : ");
  // Serial.println(UK);
  
  Serial.println("------------------------");

 if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 300000 || sendDataPrevMillis == 0)){
    //sendDataPrevMillis = millis();
    //Firebase.RTDB.setString(&fbdo, "historical_data/" + String(dateString) + "/Date", dateString);
    if (getLocalTime(&timeinfo)) {
      String timeString = String(timeinfo.tm_hour < 10 ? ("0" + String(timeinfo.tm_hour)) : String(timeinfo.tm_hour)) + ":";
      timeString += (timeinfo.tm_min < 10 ? ("0" + String(timeinfo.tm_min)) : String(timeinfo.tm_min)) + ":";
      timeString += (timeinfo.tm_sec < 10 ? ("0" + String(timeinfo.tm_sec)) : String(timeinfo.tm_sec));
      Serial.println("------------------------");
      Serial.println(timeString);

      String dateString;
      char buffer[9];
      sprintf(buffer, "%02d%02d%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
      dateString = buffer;
      Serial.println(dateString);

      Firebase.RTDB.setString(&fbdo, "historical_data/" + String(dateString) + "/" + String(timeString) + "/Timestamp", dateString + " " + timeString);

      // SEND LDR DATA TO FIREBASE
      if (Firebase.RTDB.setInt(&fbdo, "data/LDR", ldr_val)){
        Firebase.RTDB.setInt(&fbdo, "historical_data/" + String(dateString) + "/" + String(timeString) + "/LDR", ldr_val);
        Serial.println("PASSED");
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
      }
      else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }
    
      // SEND TEMP DATA TO FIREBASE
      if (Firebase.RTDB.setFloat(&fbdo, "data/Temperature", temp_val)){
        Firebase.RTDB.setString(&fbdo, "historical_data/" + String(dateString) + "/" + String(timeString) + "/Temperature", temp_val);
        Serial.println("PASSED"); 
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
      }
      else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }

      //SEND HUM DATA TO FIREBASE
      if (Firebase.RTDB.setFloat(&fbdo, "data/Humidity", hum_val)){
        Firebase.RTDB.setString(&fbdo, "historical_data/" + String(dateString) + "/" + String(timeString) + "/Humidity", hum_val);
        Serial.println("PASSED");
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
      }
      else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }

      //SEND SOIL DATA TO FIREBASE
      if (Firebase.RTDB.setFloat(&fbdo, "data/Soil", soil_val)){
        Firebase.RTDB.setFloat(&fbdo, "historical_data/" + String(dateString) + "/" + String(timeString) + "/Soil", soil_val);
        Serial.println("PASSED");
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
      }
      else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }

      //SEND PH TO FIREBASE
      if (Firebase.RTDB.setFloat(&fbdo, "data/pH", nilaipH)){
        Firebase.RTDB.setFloat(&fbdo, "historical_data/" + String(dateString) + "/" + String(timeString) + "/pH", nilaipH);
        Serial.println("PASSED");
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
      }
      else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }
      //SEND NPK TO FIREBASE

      if (Firebase.RTDB.setFloat(&fbdo, "data/unsurN", Nx)){
        Firebase.RTDB.setFloat(&fbdo, "historical_data/" + String(dateString) + "/" + String(timeString) + "/unsurN", Nx);
        Serial.println("PASSED");
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
      }
      else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }

      if (Firebase.RTDB.setFloat(&fbdo, "data/unsurP", Px)){
        Firebase.RTDB.setFloat(&fbdo, "historical_data/" + String(dateString) + "/" + String(timeString) + "/unsurP", Px);
        Serial.println("PASSED");
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
      }
      else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }

      if (Firebase.RTDB.setFloat(&fbdo, "data/unsurK", Kx)){
        Firebase.RTDB.setFloat(&fbdo, "historical_data/" + String(dateString) + "/" + String(timeString) + "/unsurK", Kx);
        Serial.println("PASSED");
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
      }
      else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }

      
      } else {
        Serial.println("No time available (yet)");
      }
      
    }
    // if (Firebase.RTDB.setFloat(&fbdo, "data/NPK", npk_val)){
    //   //Firebase.RTDB.setFloat(&fbdo, "historical_data/" + String(timeString) + "/Soil", soil_val);
    //   Serial.println("PASSED");
    //   Serial.println("PATH: " + fbdo.dataPath());
    //   Serial.println("TYPE: " + fbdo.dataType());
    // }
    // else {
    //   Serial.println("FAILED");
    //   Serial.println("REASON: " + fbdo.errorReason());
    // }

    //SEND HISTORICAL DATA
    // if (Firebase.RTDB.setString(&fbdo, "Historical_Data/", dateString)){
    //   //Firebase.RTDB.setFloat(&fbdo, "historical_data/" + String(timeString) + "/Soil", soil_val);
    //   Serial.println("PASSED");
    //   Serial.println("PATH: " + fbdo.dataPath());
    //   Serial.println("TYPE: " + fbdo.dataType());
    // }
    // else {
    //   Serial.println("FAILED");
    //   Serial.println("REASON: " + fbdo.errorReason());
    //}
    delay(10000);  
  }

  



