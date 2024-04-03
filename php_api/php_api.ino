#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include "Wire.h"
#include "PN532_I2C.h"
#include "PN532.h"

const char* SSID = "NOLA 37G";
const char* PASS = "12345678";
String url = "https://50edf5f5-9bd1-4f0f-9a07-e705dcfdd7e4-00-3jxs55jben8s.pike.replit.dev/?username=habito_001";

PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

String userRFID = "";

void setup() {
  
  Serial.begin(115200);
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

  //  Non-blocking procedure
  nfc.setPassiveActivationRetries(0x01);
 
  // configure board to read RFID tags
  nfc.SAMConfig();

  Serial.println("Waiting for an ISO14443A Card ...");
  
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
void loop() {
  readRFID();
}

void readRFID(void)
{
  boolean success;
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

  if (success)
  {
    for (uint8_t i = 0; i < uidLength; i++)
    {
      if (uid[i] <= 0xF) {
        userRFID += "0";
      }
      userRFID += String(uid[i] & 0xFF, HEX);
    }
    userRFID.toUpperCase();
    Serial.println(userRFID);
getData();
    userRFID = "";
  }
  Serial.println(success);
  delay(10000);
}

void getData(){
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    
    client.setInsecure();
    
    HTTPClient https;
    
    String fullUrl = url;
    Serial.println(" ");
    Serial.println("Requesting " + fullUrl);
    if (https.begin(client, fullUrl)) {
      int httpCode = https.GET();
      Serial.println("Respon : " + String(httpCode));
      if (httpCode > 0) {
        Serial.println(https.getString());
      }
      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }
}
