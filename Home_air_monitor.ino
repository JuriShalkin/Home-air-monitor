/**************************************************************
 * Blynk is a platform with iOS and Android apps to control
 * Arduino, Raspberry Pi and the likes over the Internet.
 * You can easily build graphic interfaces for all your
 * projects by simply dragging and dropping widgets.
 *
 *   Downloads, docs, tutorials: http://www.blynk.cc
 *   Blynk community:            http://community.blynk.cc
 *   Social networks:            http://www.fb.com/blynkapp
 *                               http://twitter.com/blynk_app
 *
 * Blynk library is licensed under MIT license
 * This example code is in public domain.
 *
 **************************************************************
 * This example runs directly on ESP8266 chip.
 *
 * Note: This requires ESP8266 support package:
 *   https://github.com/esp8266/Arduino
 *
 * Please be sure to select the right ESP8266 module
 * in the Tools -> Board menu!
 *
 * Change WiFi ssid, pass, and Blynk auth token to run :)
 *
 **************************************************************/

#include "SparkFunHTU21D.h"
#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SimpleTimer.h>
#include <SoftwareSerial.h>



#define RELE D7
#define BLYNK_PRINT Serial
#define ONE_WIRE_BUS D4
#define MH_Z19_RX       D5  //AKA GPIO14
#define MH_Z19_TX       D8  //AKA GPIO15


HTU21D myHumidity;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors (&oneWire);

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "";

SimpleTimer timer;
bool rele;

SoftwareSerial co2Serial(MH_Z19_RX, MH_Z19_TX); // define MH-Z19

// Your WiFi credentials.
// Set password to "" for open networks.
  char ssid[] = "";
  char pass[] = "";

void myTimerEvent()
{
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  float h = myHumidity.readHumidity();
  int ppm = readCO2();
  Serial.print(temp, 2);
  Serial.print(", ");
  Serial.print(h, 2);
  Serial.print(", ");
  Serial.println(ppm);
  Blynk.virtualWrite(V1, temp);
  Blynk.virtualWrite(V2, h);
  Blynk.virtualWrite(V3, ppm);
}

void setup()
{
  digitalWrite(RELE, HIGH); 
  pinMode(RELE, OUTPUT);
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);
  co2Serial.begin(9600); //Init sensor MH-Z19(14)
  timer.setInterval(10000L, myTimerEvent);
  sensors.begin();
  //dht.begin();
  myHumidity.begin();
 }

BLYNK_WRITE(V0)
{
rele = param.asInt();
digitalWrite(RELE, rele);  
}

void loop()
{
  Blynk.run();
  timer.run(); // Initiates SimpleTimer
}

int readCO2(){

  byte cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79}; // command to ask for data
  byte response[9]; // for answer

  co2Serial.write(cmd, 9); //request PPM CO2

  // The serial stream can get out of sync. The response starts with 0xff, try to resync.
  while (co2Serial.available() > 0 && (unsigned char)co2Serial.peek() != 0xFF) {
    co2Serial.read();
  }

  memset(response, 0, 9);
  co2Serial.readBytes(response, 9);

  if (response[1] != 0x86) {
    Serial.println("Invalid response from co2 sensor!");
    return -1;
  }

  byte crc = 0;
  for (int i = 1; i < 8; i++) {
    crc += response[i];
  }
  crc = 255 - crc + 1;

  if (response[8] == crc) {
    int responseHigh = (int) response[2];
    int responseLow = (int) response[3];
    int ppm = (256 * responseHigh) + responseLow;
    return ppm;
  } else {
    Serial.println("CRC error!");
    return -1;
  }
}
