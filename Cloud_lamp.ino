#include <OLEDDisplay.h>
#include <OLEDDisplayFonts.h>
#include <OLEDDisplayUi.h>
#include <SH1106.h>
//#include <SH1106Brzo.h>
#include <SH1106Wire.h>
#include <SSD1306.h>
//#include <SSD1306Brzo.h>
#include <SSD1306Spi.h>
#include <SSD1306Wire.h>

//Pin   Function  ESP-8266 Pin
//TX  TXD   TXD
//RX  RXD   RXD
//A0  Analog input, max 3.3V input  A0
//D0  IO  GPIO16
//D1  IO, SCL   GPIO5
//D2  IO, SDA   GPIO4
//D3  IO, 10k Pull-up   GPIO0
//D4  IO, 10k Pull-up, BUILTIN_LED  GPIO2
//D5  IO, SCK   GPIO14
//D6  IO, MISO  GPIO12
//D7  IO, MOSI  GPIO13
//D8  IO, 10k Pull-down, SS   GPIO15
//G   Ground  GND
//5V  5V  -
//3V3   3.3V  3.3V
//RST   Reset   RST

//#define LCD_DISP
#define OLED_DISP

#include <Wire.h>
#ifdef LCD_DISP
#include <LiquidCrystal_I2C.h>
#endif
#ifdef OLED_DISP
//#include <SH1106.h>

#include "images.h"
#endif

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager 
const char HTTP_CLOUD_SCRIPT[] PROGMEM = "<script>function showVal(m){var xhttp=new XMLHttpRequest; xhttp.open('POST','/',true); xhttp.send('m='+m);}</script>";
const char HTTP_CLOUD_MENU[] PROGMEM  = "<form action=\"/\" name=\"on\"     method=\"POST\">   <input type=\"hidden\"><button type=\"submit\"  value=\"on\">On    </button></form><br/><form action=\"/off\" name=\"off\"    method=\"POST\">   <input type=\"hidden\"><button type=\"submit\"  value=\"off\">Off   </button></form><br/><form action=\"/\" name=\"storm\"  method=\"POST\">   <input type=\"hidden\"><button type=\"submit\"  value=\"storm\">Storm </button></form><br/><!--range--><br/><form action=\"/\" name=\"sleep\"  method=\"POST\">  <input type=\"hidden\"><button type=\"submit\"  value=\"sleep\">Sleep (30 min)</button></form>";
const char HTTP_CLOUD_RANGE[] PROGMEM = "<br/><input type='range' min='10' max='1000' step='10' onchange='showVal(this.value)'>";

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
const char* host = "CloudLamp";
typedef void (*Flash)(void);

#ifdef LCD_DISP
// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);   //0x3f (0x38 - 0x3f)
//  lcd.begin(13, 12);  // sda=D7, scl=D6
//  lcd.backlight(); //lcd.noBacklight();
#endif
#ifdef OLED_DISP
// Initialize the OLED display using Wire library
SH1106 display(0x3c, 13, 12); //address, sda=D7, scl=D6
//SH1106 display(13, 12, 0x3c);
#endif

//    FILE: lightning.pde
//  AUTHOR: Rob Tillaart
//    DATE: 2012-05-08
//
// PUPROSE: simulate lighning POC
//

int multiplier = 500;
int BETWEEN = 1000 + 359 * multiplier;
#define DURATION 60 //43
#define TIMES 4

#define LEDPIN 13
int lightON = HIGH;
int lightOFF = LOW;

unsigned long lastTime1 = 0;
int waitTime1 = 0;
unsigned long lastTime2 = 0;
int waitTime2 = 0;
unsigned long lastTime3 = 0;
int waitTime3 = 0;
unsigned long lastTime4 = 0;
int waitTime4 = 0;
//unsigned long lastFadeTime1 = 0;
//int waitFadeTime1 = 0;

int mode = 2; //2 - on
//0 - off
//1 - storm
//3 - sleep

unsigned long sleepPreviousMillis = 0;
const unsigned long sleepTimeInterval = 1800000; //30 min
unsigned long sleepTimeEnd = 0;
const int buttonPin = 16;    // the number of the pushbutton pin

String getPage(){
  String page = "<html lang='en'><head><meta http-equiv='refresh' content='60' name='viewport' content='width=device-width, initial-scale=1'/>";
  page += "<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/3.1.1/jquery.min.js'></script><script src='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js'></script>";
  page += "<title>Cloud Lamp</title></head><body>";
  page += "<script>function showVal(m){var xhttp=new XMLHttpRequest; xhttp.open('POST','/storm',true); xhttp.send('m='+m);}</script>";
  page += "<form action='/' name='on'     method='get'>   <button>On    </button></form><br/>";
  page += "<form action='/' name='off'    method='get'>   <button>Off   </button></form><br/>";
  page += "<form action='/' name='storm'  method='get'>   <button>Storm </button></form><br/>";
  page += "<input type='range min='10' max='1000' step='10' onchange='showVal(this.value)'><br/>";
  page += "<form action='/' name='sleep'  method='post'>  <button>Sleep (30 min)</button></form>";
  page += "</body></html>";
  return page;
}

void fadeout( int ledPin ) {
  for (int fadeValue = 1023 ; fadeValue >= 0; fadeValue -= 20) {
    // sets the value (range from 0 to 1023 // 254 for arduino modules):
    analogWrite(ledPin, fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    delay(random(5, 20));
  }
  analogWrite(ledPin, 0);
  digitalWrite(ledPin, lightOFF);
}
void fadein( int ledPin ) {
  for (int fadeValue = 0 ; fadeValue <= 1000; fadeValue += 20) {
    // sets the value (range from 0 to 1023 // 254 for arduino modules):
    analogWrite(ledPin, fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    delay(random(5, 20));
  }
  analogWrite(ledPin, 1023);
  digitalWrite(ledPin, lightON);
}

void sleep_light() {
  if ((millis() > sleepTimeEnd) || (millis() < sleepPreviousMillis)) // check timeout and mills rollover
  {
    digitalWrite(D1, lightOFF);
  }
  sleepPreviousMillis = millis();
}

void clear_storm_wait() {
  lastTime1 = millis();
  lastTime2 = millis();
  lastTime3 = millis();
  lastTime4 = millis();
  waitTime1 = 0;
  waitTime2 = 0;
  waitTime3 = 0;
  waitTime4 = 0;
}

void flash1()
{
  if (millis() - waitTime1 > lastTime1)  // time for a new flash
  {
    // adjust timing params
    lastTime1 += waitTime1;
    waitTime1 = random(BETWEEN);
    int times_local = random(1, TIMES);
    for (int i = 0; i < times_local; i++)
    {
      Serial.print("1");
      digitalWrite(D1, lightON);
      delay(20 + random(DURATION));
      if (times_local == 1) {
        fadeout(D1);
      }
      else {
        digitalWrite(D1, lightOFF);
        delay(20 + random(DURATION));
      }
    }
  }
}

void flash2()
{
  if (millis() - waitTime2 > lastTime2)  // time for a new flash
  {
    // adjust timing params
    lastTime2 += waitTime2;
    waitTime2 = random(BETWEEN);
    int times_local = random(1, TIMES);
    for (int i = 0; i < times_local; i++)
    {
      Serial.print("2");
      digitalWrite(D2, lightON);
      delay(20 + random(DURATION));
      if (times_local == 1) {
        fadeout(D2);
      }
      else {
        digitalWrite(D2, lightOFF);
        delay(20 + random(DURATION));
      }
    }
  }
}

void flash3()
{
  if (millis() - waitTime3 > lastTime3)  // time for a new flash
  {
    // adjust timing params
    lastTime3 += waitTime3;
    waitTime3 = random(BETWEEN);
    int times_local = random(1, TIMES);
    for (int i = 0; i < times_local; i++)
    {
      Serial.print("3");
      digitalWrite(D3, lightON);
      delay(20 + random(DURATION));
      if (times_local == 1) {
        fadeout(D3);
      }
      else {
        digitalWrite(D3, lightOFF);
        delay(20 + random(DURATION));
      }
    }
  }
}
void flash4()
{
  if (millis() - waitTime4 > lastTime4)  // time for a new flash
  {
    // adjust timing params
    lastTime4 += waitTime4;
    waitTime4 = random(BETWEEN);
    int times_local = random(1, TIMES);
    for (int i = 0; i < times_local; i++)
    {
      Serial.print("4");
      digitalWrite(D4, lightON);
      delay(20 + random(DURATION));
      if (times_local == 1) {
        fadeout(D4);
      }
      else {
        digitalWrite(D4, lightOFF);
        delay(20 + random(DURATION));
      }
    }
  }
}
void lightoff() {
  digitalWrite(D1, lightOFF);
  digitalWrite(D2, lightOFF);
  digitalWrite(D3, lightOFF);
  digitalWrite(D4, lightOFF);
//  fadeout(D1);
//  fadeout(D2);
//  fadeout(D3);
//  fadeout(D4);
//  page = FPSTR(HTTP_HEAD);
//  //page.replace("{v}", "CloudLamp");
//  page += FPSTR(HTTP_STYLE);
//  page += FPSTR(HTTP_HEADER_END);
//  page += FPSTR(HTTP_CLOUD_MENU);
//  page += FPSTR(HTTP_END);
//  server.send ( 200, "text/html", page );
  yield();
  Serial.println("Light mode off");
  mode = 0;
}

void storm() {

//  page = FPSTR(HTTP_HEAD);
// // page.replace("{v}", "CloudLamp");
//  page += FPSTR(HTTP_CLOUD_SCRIPT);
//  page += FPSTR(HTTP_STYLE);
//  //        page += _customHeadElement;
//  page += FPSTR(HTTP_HEADER_END);
//  //          page += "<h1>";
//  //          page += WiFi.localIP().toString();
//  //          page += "</h1>";
//  //          page += F("<h3>WiFiManager</h3>");
//  page += FPSTR(HTTP_CLOUD_MENU);
//  //page.replace("<!--range-->", FPSTR(HTTP_CLOUD_RANGE));
//  page += FPSTR(HTTP_END);
//  server.send(200, "text/html", page);
//  
  clear_storm_wait();

  if (server.arg(0).toInt() >= 1) {
    multiplier = server.arg(0).toInt();
  }
  BETWEEN = 1000 + 359 * multiplier;

  Serial.print("\nBETWEEN time is ");
  Serial.print(" = ");
  Serial.println(BETWEEN / 1000);
  
  //yield();
  Serial.println("Storm mode activated");
  mode = 1;
}
void lighton() {
  digitalWrite(D1, lightON);
  digitalWrite(D2, lightON);
//  digitalWrite(D3, lightON);
//  digitalWrite(D4, lightON);
//  fadein(D1);
//  fadein(D2);
//  fadein(D3);
//  fadein(D4);

//  page = FPSTR(HTTP_HEAD);
//  //page.replace("{v}", "CloudLamp");
//  page += FPSTR(HTTP_STYLE);
//  page += FPSTR(HTTP_HEADER_END);
//  page += FPSTR(HTTP_CLOUD_MENU);
//  page += FPSTR(HTTP_END);
//  server.send ( 200, "text/html", page );
  
  //yield();
  Serial.println("Light mode on");
  mode = 2;
}
void sleep() {
  if (mode != 3) {
    sleepTimeEnd = millis() + sleepTimeInterval;
  }

  digitalWrite(D1, lightON);
  digitalWrite(D2, lightOFF);
  digitalWrite(D3, lightOFF);
  digitalWrite(D4, lightOFF);

//  page = FPSTR(HTTP_HEAD);
//  //page.replace("{v}", "CloudLamp");
//  page += FPSTR(HTTP_STYLE);
//  page += FPSTR(HTTP_HEADER_END);
//  page += FPSTR(HTTP_CLOUD_MENU);
//  page += FPSTR(HTTP_END);
//  server.send ( 200, "text/html", page );
//  
  //yield();
  Serial.println("Sleep mode");
  mode = 3;
}

void handleRoot() {
  
  String page = FPSTR(HTTP_HEADER);
  page.replace("{v}", "CloudLamp");
//  page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_CLOUD_SCRIPT);
  page += FPSTR(HTTP_STYLE);
  page += FPSTR(HTTP_HEADER_END);
  page += FPSTR(HTTP_CLOUD_MENU);
  page += FPSTR(HTTP_END);

  if ( server.hasArg("on") ) {
    lighton;
    Serial.println("Has argument ON");
  }
  else if ( server.hasArg("off") ) {
    lightoff;
    Serial.println("Has argument OFF");
  }
  else if ( server.hasArg("storm") ) {
    page.replace("<!--range-->", FPSTR(HTTP_CLOUD_RANGE));
    storm;
    Serial.println("Has argument STORM");
  }
  else if ( server.hasArg("sleep") ) {
    sleep;
    Serial.println("Has argument SLEEP");
  }
  
  Serial.println(server.args());
  
  String arguments;
  for (uint8_t i = 0; i < server.args(); i++) {
    arguments += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  Serial.println(arguments);
  Serial.println("Root page accessed");
  server.send ( 200, "text/html", page );

//  server.send ( 200, "text/html", getPage() );
  
//  yield();
}

void handleNotFound(){
  Serial.println("Page not found. Handled by /");
  handleRoot;
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
#ifdef LCD_DISP
  lcd.noBlink();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("AP:");
  lcd.setCursor(3, 0);
  lcd.print(myWiFiManager->getConfigPortalSSID());
  lcd.setCursor(0, 1);
  lcd.print("IP:");
  lcd.setCursor(3, 1); //second line
  lcd.print(WiFi.softAPIP().toString());
#endif
#ifdef OLED_DISP
  display.clear();
  display.drawXbm(0, 0, Cloud_width, Cloud_height, Cloud_bits);
  // The coordinates define the center of the text
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 0, "CloudLamp");
  // The coordinates define the left starting point of the text
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10); //10 16 24
  display.drawStringMaxWidth(36, 12, 95, "Connect to AP" ); //http://" + WiFi.localIP().toString() + "/");
  display.setFont(ArialMT_Plain_16); //10 16 24
  display.drawString(20, 25, myWiFiManager->getConfigPortalSSID());
  display.setFont(ArialMT_Plain_16); //10 16 24
  display.drawStringMaxWidth(0, 45, 127, "http://" + WiFi.softAPIP().toString() );
  display.display();
#endif
}

void setup()
{
  Serial.begin(115200);
  pinMode(buttonPin, INPUT);
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
//  digitalWrite(D1, lightON);  //-> couch side internal
  digitalWrite(D2, lightON);    //->lower
  digitalWrite(D3, lightON);    //->max power periferal
//  digitalWrite(D4, lightON);
//  fadein(D1);
//  fadein(D2);
//  fadein(D3);
//  fadein(D4);

#ifdef LCD_DISP
  // LCD
  //  Wire.begin(13, 12);          //sda, scl
  // initialize the LCD
  lcd.begin(12, 13);  // sda=D6, scl=D7
  lcd.backlight(); //lcd.noBacklight();
  lcd.setCursor(3, 0);
  lcd.print("CloudLamp");
  lcd.setCursor(0, 1); //second line
  lcd.print("connecting WiFi");
  lcd.blink();
#endif
#ifdef OLED_DISP
  display.init();
  //display.begin();
  display.flipScreenVertically();
  display.drawXbm(0, 0, Cloud_width, Cloud_height, Cloud_bits);
  display.setFont(ArialMT_Plain_10);
  // The coordinates define the center of the text
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 0, "CloudLamp");
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawStringMaxWidth(36, 12, 95,
                             "connecting WiFi" );
  display.display();
#endif
  wifi_station_set_hostname("CloudLamp");
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;  // commented here because it is initiallized outside setup()
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
//  if (!wifiManager.autoConnect("CloudLamp")) {
//    Serial.println("failed to connect and hit timeout");
//    //reset and try again, or maybe put it to deep sleep
//    ESP.reset();
//    delay(1000);
//    //if you get here you have connected to the WiFi
//    Serial.println("connected...yeey :)");
//  }

  wifiManager.autoConnect("CloudLamp");

  WiFi.hostname(host);
//  if (!MDNS.begin(host)) {
//    Serial.println("Error setting up MDNS responder!");
//  }

  MDNS.addService("http", "tcp", 80);
  httpUpdater.setup(&server);
  server.on( "/lightoff" , lightoff );
  server.on( "/storm" , storm );
  server.on( "/lighton" , lighton );
  server.on( "/sleep" , sleep );
  server.on( "/" , handleRoot );
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.print ( "HTTP server started on IP " );
  Serial.println ( WiFi.localIP().toString());
  Serial.printf ( "Update URL is http://%s.local/update in your browser\n", host  );
  Serial.print (F(__DATE__)) ;
  Serial.print (" ") ;
  Serial.println (F(__TIME__)) ;
#ifdef LCD_DISP
  lcd.noBlink();
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("CloudLamp");
  lcd.setCursor(0, 1); //second line
  lcd.print(WiFi.localIP().toString());
#endif
#ifdef OLED_DISP
  display.clear();
  display.drawXbm(0, 0, Cloud_width, Cloud_height, Cloud_bits);
  display.setFont(ArialMT_Plain_10); //10 16 24
  // The coordinates define the center of the text
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 0, "CloudLamp");
  // The coordinates define the left starting point of the text
  //  display.setTextAlignment(TEXT_ALIGN_LEFT);
  //  display.setFont(ArialMT_Plain_16); //10 16 24
  //  display.drawStringMaxWidth(0, 45, 127, "http://" + WiFi.localIP().toString() + "/");
  display.drawString(63, 51, "http://" + WiFi.localIP().toString() + "/");
  display.display();
#endif
}
Flash flashes[] = { flash1, flash2, flash3, flash4 };
void loop()
{

//  yield();
  server.handleClient();  
//  yield();

  switch (mode) {
    case 0:             //0 - off

      break;
    case 1:             //1 - storm
      flashes[random(4)]();
      //      flash1();
      //      flash2();
      //      flash3();
      //      flash4();
      break;
    case 2:             //2 - on
//      digitalWrite(D1, lightON);
//      delay(80);
//      digitalWrite(D2, lightON);
//      delay(80);
//      digitalWrite(D3, lightON);
//      delay(80);
//      digitalWrite(D4, lightON);

      //      clear_storm_wait();
      break;
    case 3:             //3 - sleep
      sleep_light();
      //      clear_storm_wait();
      break;
    default:
      // statements
      break;
  }

  // do other stuff here
//
//  while (Serial.available()) {
//
//    String serial_command = Serial.readString();
//    Serial.println (serial_command);
//    if (serial_command == "IP" || serial_command == "ip" )
//    {
//      Serial.println ( WiFi.localIP().toString());
//    }
//    else if (serial_command == "storm") {
//      storm();
//    }
//    else if (serial_command == "on") {
//      lighton();
//    }
//    else if (serial_command == "sleep") {
//      sleep();
//    }
//    else if (serial_command == "off") {
//      lightoff();
//    }
////    else if (serial_command == "resetwifi") {
////      WiFiManager wifiManager;
////      wifiManager.resetSettings();
////      Serial.println ( "Resetting WiFi settings..." );
////      ESP.restart();
////    }
//  }

  //  if (digitalRead(buttonPin) == HIGH)
  //  {
  //    Serial.println("Resetting WiFi settings");
  //    wifiManager.resetSettings();
  //    ESP.restart();
  //  }

}
