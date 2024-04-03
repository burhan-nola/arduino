#include "Wire.h"
#include "PN532_I2C.h"
#include "PN532.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

const char* SSID = "NOLA 37G";
const char* PASS = "12345678";
String idDevice = "habito_002"; //change this based on the device id
String url = "https://habito-api.vercel.app"; //this is the API url


PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

String userRFID = "";
unsigned long previousMillis = 0;
const long interval = 1000; // Interval antara pembacaan RFID

const int BUTTON_PIN = D3; 
const int SHORT_PRESS_TIME = 1000; 
int lastState = HIGH;
int currentState;
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;
int buton = D3;

void setup()
{
  Serial.begin(115200);
  pinMode(buton,INPUT);
  Serial.println("NFC/RFID Reader");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Non-blocking procedure
//  nfc.setPassiveActivationRetries(0x01);

  // configure board to read RFID tags
  nfc.SAMConfig();

  Serial.println("Waiting for an ISO14443A Card ...");
  connectWiFi();
  regDevice();
  logs();
}

unsigned long prevOnline = 0;
const long intervalOnline = 20000;

void loop()
{
trial();

}


void readRFID()
{
  static unsigned long lastRFIDPollTime = 0;
  const unsigned long RFIDPollInterval = 5000; // Polling setiap 1 detik
  unsigned long currentMillis = millis();

  if (currentMillis - lastRFIDPollTime >= RFIDPollInterval) {
    lastRFIDPollTime = currentMillis;
    
      uint8_t success;
      uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
      uint8_t uidLength;
    
      success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
    
      if (success) {
        // Kartu terdeteksi
        Serial.println("Kartu Terdeteksi");
    
        // Dapatkan UID kartu
        String cardUID = "";
        for (uint8_t i = 0; i < uidLength; i++) {
          cardUID += String(uid[i], HEX);
        }
    
        Serial.print("UID Kartu: ");
        Serial.println(cardUID);
        Serial.println(success);
    
        // Kirim data ke API
        trial();
        delay(500); // Delay sebelum membaca kartu berikutnya
      }
  }
}


void connectWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); 
}

void regDevice(){
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["id"] = idDevice;

  // Serialize the JSON document to a string
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    String endpoint = "/register";
    String query = "?id=";

    String fullUrl = url + endpoint;

    Serial.print("Requesting: ");
    Serial.println(fullUrl);

    if (https.begin(client, fullUrl)) {
      https.addHeader("Content-Type", "application/json");
        int httpCode = https.POST(jsonString);
        Serial.print("HTTP Response Code: ");
        Serial.println(httpCode);
        
          DynamicJsonDocument doc(1024);
          deserializeJson(doc, https.getString());
          String message = doc["message"];
          
        if (httpCode > 0) {
            Serial.println("Response Body: ");
            Serial.println(message);
        }
        https.end();
    } else {
        Serial.println("[HTTPS] Unable to connect");
    }
}

void logs() {
  IPAddress ipAddress = WiFi.localIP();
  String ip = ipAddress.toString();
  
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["id"] = idDevice;
  jsonDoc["ip"] = ip;
  jsonDoc["ssid"] = SSID;

  // Serialize the JSON document to a string
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;

    String endpoint = "/logs";
    String query = "?id=";

    String fullUrl = url + endpoint;
    
    Serial.print("Requesting: ");
    Serial.println(fullUrl);
    
    if (https.begin(client, fullUrl)) {
      https.addHeader("Content-Type", "application/json");
        int httpCode = https.POST(jsonString);
        Serial.print("HTTP Response Code: ");
        Serial.println(httpCode);
        if (httpCode > 0) {
          DynamicJsonDocument doc(1024);
          deserializeJson(doc, https.getString());
          String message = doc["message"];
          
            Serial.println("Response Body: ");
            Serial.println(message);
        }
        https.end();
    } else {
        Serial.println("[HTTPS] Unable to connect");
    }
}

void online() {
  char ip[16]; // Menyimpan alamat IP sebagai string
  WiFi.localIP().toString().toCharArray(ip, 16); // Mendapatkan alamat IP dan mengonversinya menjadi string
  
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["id"] = idDevice;
  jsonDoc["ip"] = ip;
  jsonDoc["ssid"] = SSID;

  char jsonString[200]; // Buffer untuk menyimpan string JSON
  serializeJson(jsonDoc, jsonString, sizeof(jsonString)); // Serialisasi JSON

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  String endpoint = "/keep-online";

  String fullUrl = url + endpoint;

  Serial.print("Requesting: ");
  Serial.println(fullUrl);

  if (https.begin(client, fullUrl)) {
    https.addHeader("Content-Type", "application/json");
    int httpCode = https.POST(jsonString);
    Serial.print("HTTP Response Code: ");
    Serial.println(httpCode);

    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, https.getString());
        String deviceStatus = doc["message"];// Ambil status perangkat dari respons JSON
        Serial.println(deviceStatus);
      }
    } else {
      Serial.println(httpCode);
    }
    https.end();
  } else {
    Serial.println("[HTTPS] Unable to connect");
  }
}

void trial(){
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  //String url = "http://192.168.0.19/emsai/updatewemos.php?room="+String(room)+"&status=0";
  String url = "https://habito-api.vercel.app/data?uid=1212";
  Serial.println(url);     
  http.begin(client,url);
 
  //GET method
  int httpCode = http.GET();
  if(httpCode > 0)
  {
    Serial.printf("[HTTP] GET...code: %d\n", httpCode);
    if(httpCode == HTTP_CODE_OK)
    {
        String payload = http.getString();
        Serial.println(payload);
    }
 }
 else
 {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
 }
    http.end();
    delay(5000);
}
