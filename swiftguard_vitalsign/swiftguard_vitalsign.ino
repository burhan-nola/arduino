#include "MAX30105.h"
#include "heartRate.h"
#include <Wire.h>
#include <WiFi.h>    
#include <HTTPClient.h>
#include <UrlEncode.h>
const char* ssid = "swiftguard";
const char* password = "lombai2aspo";
WiFiServer server(80); 
WiFiClient wifiClient;

// #include <Adafruit_MLX90614.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET     -1 

int check = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MAX30105 particleSensor;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;

long samplesTaken = 0; //Counter for calculating the Hz or read rate
long unblockedValue; //Average IR at power up
long startTime; //Used to calculate measurement rate

int perCent; 
int degOffset = 1; //calibrated Farenheit degrees
int irOffset = 2100;
int count;
int noFinger;
//auto calibrate
int avgIr;
int avgTemp;

void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing...");

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);    
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,25);  
  display.println(F("SWIFTGUARD"));
  display.display();
  delay(2000); 

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  display.setCursor(20,45);  
  display.setTextSize(1);
  display.println(F("Connecting"));
  display.display();
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  display.setTextSize(1);
  display.setCursor(20,55);  
  display.println(F(" connected "));
  display.display();

  //starting the server
    server.begin();
    Serial.println("Server started.");

    //get the ip address and print it
    Serial.print("This is your ip address: ");
    Serial.print("http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");  

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  //The LEDs are very low power and won't affect the temp reading much but
  //you may want to turn off the LEDs to avoid any local heating
  particleSensor.setup(0); //Configure sensor. Turn off LEDs
  particleSensor.enableDIETEMPRDY(); //Enable the temp ready interrupt. This is required.
  
  //Setup to sense up to 18 inches, max LED brightness
  byte ledBrightness = 25; //Options: 0=Off to 255=50mA=0xFF hexadecimal. 100=0x64; 50=0x32 25=0x19
  byte sampleAverage = 8; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  int sampleRate = 1000; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 2048; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
  
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
  particleSensor.enableDIETEMPRDY(); //Enable the temp ready interrupt. This is required.



  //Arduino plotter auto-scales annoyingly. To get around this, pre-populate
  //the plotter with 500 of an average reading from the sensor

  //Take an average of IR readings at power up
  const byte avgAmount = 64;
  long baseValue = 0;
  for (byte x = 0 ; x < avgAmount ; x++)
  {
    baseValue += particleSensor.getIR(); //Read the IR value
  }
  baseValue /= avgAmount;

  //Pre-populate the plotter so that the Y scale is close to IR values
  for (int x = 0 ; x < 500 ; x++) {
    Serial.println(baseValue);
  }
}

void loop()
{
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);


  Serial.print(" ");
  perCent = irValue / irOffset; //offset provides percent
  Serial.print("Oxygen=");
  Serial.print(perCent);
  Serial.print("%");
  //Serial.print((float)samplesTaken / ((millis() - startTime) / 1000.0), 2);

  float temperature = particleSensor.readTemperature(); //Because I am a bad global citizen
  temperature = temperature + degOffset;
  
  Serial.print(" Temp(C)=");
  Serial.print(temperature, 2);
  Serial.print("°");

  Serial.print(" IR=");
  Serial.print(irValue);

  if (irValue < 50000) {
    Serial.print(" No finger?");
    noFinger = noFinger+1;
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(35,5); 
    display.print("No Finger");
    display.display();
    check = 0;

  } else {
    //only count and grab the reading if there's something there.
    if(check == 0)
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(30,5); 
      display.print("Calculation");
      display.display();
    }
    check = 1;
    count = count+1;
    avgIr = avgIr + irValue;
    avgTemp = avgTemp + temperature;
    

  }
  

  //Get an average IR value over 100 loops
  if (count == 500) {
    avgIr = avgIr / count;
    avgIr = avgIr / irOffset; //offset provides percent
    avgTemp = avgTemp / count;
    Serial.print(" avgO=");
    Serial.print(avgIr);
    Serial.print(" avgF=");
    Serial.print(avgTemp);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10,5); 
  display.print("BPM");
  display.setCursor(50,5); 
  display.print("SPO2");
  display.setCursor(95,5); 
  display.print("Temp");
  display.setTextSize(2);
  display.setCursor(5,40); 
  display.print(beatAvg+10);
  display.setCursor(50,40); 
  display.print(avgIr+10);
  display.setCursor(95,40); 
  display.print(avgTemp);

  display.display(); 
  uploadDB(3, beatAvg+10, avgIr+10, avgTemp);

delay(5000);
    Serial.print(" count=");
    Serial.print(count);
    //reset for the next 100
    count = 0; 
    avgIr = 0;
    avgTemp = 0;
  }

  //turn off the LED if there's no finger
  //this doesn't work yet.
  //if (noFinger == 500) {
  //  particleSensor.setPulseAmplitudeRed(0);
    //delay(5000);
  //} 
  //Serial.print(noFinger);

  //Heart Beat Plotter
  //Serial.println(particleSensor.getIR()); //Send raw data to plotter
  
  Serial.println();
}

void uploadDB(int user, int bpm, int spo2, int suhu)
{
  
  HTTPClient http;
        //String url = "http://192.168.0.19/emsai/updatewemos.php?room="+String(room)+"&status=0";
        String url = "http://smart.iot.web.id/updatewemos.php?user="+String(user)+"&bpm="+String(bpm)+"&spo2="+String(spo2)+"&temp="+String(suhu);
        Serial.println(url);     
        http.begin(wifiClient,url);
       
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
 