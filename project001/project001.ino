#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include "Wire.h"
#include "PN532_I2C.h"
#include "PN532.h"

const char* SSID = "Ruang Guru Lantai 3";
const char* PASS = "NorthernLight2020";
WiFiServer server(80); 
String url = "https://habito-api.vercel.app";

PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

String userRFID = "";

void setup() {
  Serial.begin(115200);
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
  nfc.setPassiveActivationRetries(0x01);

  // configure board to read RFID tags
  nfc.SAMConfig();
  Serial.println("Waiting for an ISO14443A Card ...");
  
  connectWiFi();
  server.begin();
  Serial.println("Server started.");
  //get the ip address and print it
  Serial.print("This is your ip address: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}
static unsigned long lastRFIDPollTime = 0;
const unsigned long RFIDPollInterval = 10000;

void loop() {
  readRFID();
//    String uid = "habito_001";
//    getData(uid);

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

void readRFID(void)
{
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
        String uid = "habito_001";
    getData(uid);
        delay(500); // Delay sebelum membaca kartu berikutnya
      }
//      Serial.println(success);
}

void getData(String uid){
  WiFiClientSecure client;
  client.setInsecure();
  
  HTTPClient http;
  String fullUrl = "https://habito-api.vercel.app/data?uid="+uid;
  Serial.println(fullUrl);     
  http.begin(client,fullUrl);
 
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
//    delay(20000);
}
