#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

const char* SSID = "Ruang Guru Lantai 3";
const char* PASS = "NorthernLight2020";

String url = "https://habito-api.vercel.app";

void setup() {
  Serial.begin(115200);
  connectWiFi();
}

void loop() {
  getData();

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

void getData(){
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  String fullUrl = "https://habito-api.vercel.app/data?uid=121212";
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
    delay(5000);
}
