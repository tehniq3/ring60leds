/*******************************************************************************
// https://www.hackster.io/alankrantas/esp8266-ntp-clock-on-ssd1306-oled-arduino-ide-35116e
// small changes by Nicu FLORICA (niq_ro)
// v.0 - changed date mode (day.mounth not month/day)
// v.1 - added hardware switch for DST (Daylight Saving Time)
// v.2 - added WiFiManager library 
// HT16K33_v.1 - changed to HT16K33 4-digit 14-segment display
// v.2 - added month, year and scrolling name day name
// v.3 - added DHT22 (AM2302) thermometer/hygrometer
// v.5 - added a lot of animation (maybe too much)
// v.5a - added animation also at enter the date and exit of year
// v.6 - added automatic brightness due to sunrise/sunset using https://github.com/jpb10/SolarCalculator library
// WS2812b - 60 leds
// v.2 - removed HT1633 + DHT sensor, added 60leds ring (WS2812b) + moved part of display mode into subroutine
 
 *******************************************************************************/

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <Wire.h>
#include <FastLED.h>
#include <SolarCalculator.h> //  https://github.com/jpb10/SolarCalculator

#define DSTpin 14 // GPIO14 = D5, see https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
#define NUM_LEDS 60     
#define DATA_PIN 12 // GPIO12 = D6 see https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/

#define USE_LED_MOVE_BETWEEN_HOURS true

// WS2812b day / night brightness.
#define NIGHTBRIGHTNESS 4      // Brightness level from 0 (off) to 255 (full brightness)
#define DAYBRIGHTNESS 20

const long timezoneOffset = 2; // ? hours
const char          *ntpServer  = "pool.ntp.org"; // change it to local NTP server if needed
const unsigned long updateDelay = 900000;         // update time every 15 min
const unsigned long retryDelay  = 5000;           // retry 5 sec later if time query failed

unsigned long tpafisare;
unsigned long tpinfo = 60000;  // 15000 for test in video

unsigned long lastUpdatedTime = updateDelay * -1;
unsigned int  second_prev = 0;
bool          colon_switch = false;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer);

byte DST = 0;
byte DST0;
bool updated;
byte a = 0;

  unsigned int year;
  unsigned int month;
  unsigned int day;
  unsigned int hour;
  unsigned int minute;
  unsigned int second;

int i = 0;
int j = 4;

int pauzamica = 100;  // step time (speed) for animation
//int pauzamare = 3000; // step time for static info
int pauzemari = 30; // x pauzamica [ms], ex= 30x 100 if pauzamica = 100 [ms]
int pauzamedie = 300;  // step time (speed) for info 

int ora, minut;
int zi = 0;

int n = 0;

// Location - Craiova: 44.317452,23.811336
double latitude = 44.31;
double longitude = 23.81;
double transit, sunrise, sunset;


char str[6];
int ora1, ora2, oraactuala;
int m1, hr1, mn1, m2, hr2, mn2; 
//int ora, minut, secunda, rest;
int ora0, minut0;
byte ziua, luna, an;

// Change the colors here if you want.
// Check for reference: https://github.com/FastLED/FastLED/wiki/Pixel-reference#predefined-colors-list
// You can also set the colors with RGB values, for example red:
// CRGB colorHour = CRGB(255, 0, 0);
byte ics = 64;
CRGB fundal = CRGB(ics, ics, ics);
CRGB colorHour = CRGB::Red;
CRGB colorMinute = CRGB::Green;
CRGB colorSecond = CRGB::Blue;
CRGB colorHourMinute = CRGB::Yellow;
CRGB colorHourSecond = CRGB::Magenta;
CRGB colorMinuteSecond = CRGB::Cyan;
CRGB colorAll = CRGB::White;

CRGB LEDs[NUM_LEDS];

byte inel_ora;
byte inel_minut;
byte inel_secunda;
byte secundar;


void setup() {
 pinMode (DSTpin, INPUT);
  delay(500);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(LEDs, NUM_LEDS);  
  FastLED.clear();
  
     //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();
    
    //set custom ip for portal
    //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration

    wifiManager.autoConnect("AutoConnectAP");
      
    //or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

    delay(500);

  if (digitalRead(DSTpin) == LOW)
   DST = 0;
  else
   DST = 1;
  timeClient.setTimeOffset((timezoneOffset + DST)*3600);
  timeClient.begin();
  DST0 = DST; 
  
  FastLED.setBrightness(DAYBRIGHTNESS); //Number 0-255
    LEDs[32] = colorHour;   // 2 oçlock
    LEDs[47] = colorMinute; // 7 minutes
    LEDs[42] = colorSecond; // 12 seconds  

for (int i=0; i<12; i++)  // show important points
      {
        LEDs[i*5] = fundal;  
        LEDs[i*5].fadeToBlackBy(25);      
      }
  FastLED.show();
  delay(3000);
  FastLED.setBrightness(NIGHTBRIGHTNESS); //Number 0-255
  FastLED.show();
  delay(3000);
  FastLED.clear();

Soare();
if (night())
      {
        FastLED.setBrightness (NIGHTBRIGHTNESS);
        Serial.println("NIGHT !");
      }
      else
      {
      FastLED.setBrightness (DAYBRIGHTNESS);
      }
    FastLED.show();
}

void loop() {
  if (digitalRead(DSTpin) == LOW)
   DST = 0;
  else
   DST = 1;

  if (WiFi.status() == WL_CONNECTED && millis() - lastUpdatedTime >= updateDelay) {
    updated = timeClient.update();
    if (updated) {
      Serial.println("NTP time updated.");
      lastUpdatedTime = millis();
    } else {
      Serial.println("Failed to update time. Waiting for retry...");
      lastUpdatedTime = millis() - updateDelay + retryDelay;
    }
  } else {
    if (WiFi.status() != WL_CONNECTED) Serial.println("WiFi disconnected!");
  }
 
unsigned long t = millis();

year = getYear();
month = getMonth();
day = getDate();
zi = timeClient.getDay();

hour = timeClient.getHours();
minute = timeClient.getMinutes();
second = timeClient.getSeconds();
ora = hour;
minut = minute;
secundar = second;

    afisareinel();

if (DST != DST0)
{
  timeClient.setTimeOffset((timezoneOffset + DST)*3600);
  timeClient.begin();
DST0 = DST;
}
Soare();
}  // end main loop�����������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������



unsigned int getYear() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  unsigned int year = ti->tm_year + 1900;
  return year;
}

unsigned int getMonth() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  unsigned int month = ti->tm_mon + 1;
  return month;
}

unsigned int getDate() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  unsigned int month = ti->tm_mday;
  return month;
}

void Soare()
{
   // Calculate the times of sunrise, transit, and sunset, in hours (UTC)
  calcSunriseSunset(year, month, day, latitude, longitude, transit, sunrise, sunset);

m1 = int(round((sunrise + timezoneOffset + DST) * 60));
hr1 = (m1 / 60) % 24;
mn1 = m1 % 60;

m2 = int(round((sunset + timezoneOffset + DST) * 60));
hr2 = (m2 / 60) % 24;
mn2 = m2 % 60;

  Serial.print("Sunrise = ");
  Serial.print(sunrise+timezoneOffset+DST);
  Serial.print(" = ");
  Serial.print(hr1);
  Serial.print(":");
  Serial.print(mn1);
  Serial.println(" ! ");
  Serial.print("Sunset = ");
  Serial.print(sunset+timezoneOffset+DST);
  Serial.print(" = ");
  Serial.print(hr2);
  Serial.print(":");
  Serial.print(mn2);
  Serial.println(" ! ");
}

void afisareinel()
{
  if (digitalRead(DSTpin) == LOW)
   DST = 0;
  else
   DST = 1;
  if (DST != DST0)
  {
  timeClient.setTimeOffset(timezoneOffset*3600 + DST*3600);
  timeClient.begin();
  DST0 = DST;
  Soare();
  }
hour = timeClient.getHours();
minute = timeClient.getMinutes();
second = timeClient.getSeconds();
ora = hour;
minut = minute;
secundar = second;
  
  // --------- display ring clock part ------------------------------------
    for (int i=0; i<NUM_LEDS; i++)  // clear all leds
      LEDs[i] = CRGB::Black;

    int inel_secunda = getLEDMinuteOrSecond(secundar);
    int inel_minut = getLEDMinuteOrSecond(minut);
    int inel_ora = getLEDHour(ora, minut);

for (int i=0; i<12; i++)  // show important points
      {
        LEDs[i*5] = fundal;  
        LEDs[i*5].fadeToBlackBy(25);
      }

    // Set "Hands"
    LEDs[inel_secunda] = colorSecond;
    LEDs[inel_minut] = colorMinute;  
    LEDs[inel_ora] = colorHour;  

    // Hour and min are on same spot
    if ( inel_ora == inel_minut)
      LEDs[inel_ora] = colorHourMinute;

    // Hour and sec are on same spot
    if ( inel_ora == inel_secunda)
      LEDs[inel_ora] = colorHourSecond;

    // Min and sec are on same spot
    if ( inel_minut == inel_secunda)
      LEDs[inel_minut] = colorMinuteSecond;

    // All are on same spot
    if ( inel_minut == inel_secunda && inel_minut == inel_ora)
      LEDs[inel_minut] = colorAll;

    if (night())
      {
        FastLED.setBrightness (NIGHTBRIGHTNESS);
        Serial.println("NIGHT !");
      }
      else
      {
      FastLED.setBrightness (DAYBRIGHTNESS);
      }
    FastLED.show();
// ---------end of display ring clock part ------------------------------------
}

byte getLEDHour(byte orele, byte minutele) {
  if (orele > 12)
    orele = orele - 12;

  byte hourLED;
  if (orele <= 5) 
    hourLED = (orele * 5) + 30;
  else
    hourLED = (orele * 5) - 30;

  if (USE_LED_MOVE_BETWEEN_HOURS == true) {
    if        (minutele >= 12 && minutele < 24) {
      hourLED += 1;
    } else if (minutele >= 24 && minutele < 36) {
      hourLED += 2;
    } else if (minutele >= 36 && minutele < 48) {
      hourLED += 3;
    } else if (minutele >= 48) {
      hourLED += 4;
    }
  }
  return hourLED;  
}

byte getLEDMinuteOrSecond(byte minuteOrSecond) {
  if (minuteOrSecond < 30) 
    return minuteOrSecond + 30;
  else 
    return minuteOrSecond - 30;
}

boolean night() { 
  ora1 = 100*hr1 + mn1;  // rasarit 
  ora2 = 100*hr2 + mn2;  // apus
  oraactuala = 100*ora + minut;  // ora actuala

  Serial.print(ora1);
  Serial.print(" ? ");
  Serial.print(oraactuala);
  Serial.print(" ? ");
  Serial.println(ora2);  

  if ((oraactuala > ora2) or (oraactuala < ora1))  // night 
    return true;  
    else
    return false;  
}
