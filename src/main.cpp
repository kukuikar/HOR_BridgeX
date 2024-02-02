#include <GyverMotor2.h>
#include <GParser.h>
#include <Servo.h>
#include <Arduino.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <EEPROM.h>

AsyncWebServer server(80);

//#define MKD_Guest
#ifdef MKD_Guest
const char *ssid = "MKD-Guest";
const char *pswd = "123Qweasd";
#else
const char *ssid = "Keenetic-1649";
const char *password = "jsCMnJpr";
#endif

const uint32_t port = 1234;
const char *hostname = "BRIDGE";
WiFiUDP udp;

IPAddress ServIP(192, 0, 0, 0);

const int8_t BRIDGE_MOTOR_PIN_DIR = 13; ///7
const int8_t BRIDGE_MOTOR_PIN_PWM = 12; //6

const int8_t TROLLEY_MOTOR_PIN_DIR = 2; //4
const int8_t TROLLEY_MOTOR_PIN_PWM =5;  //1

const int8_t WINCH_MOTOR_PIN_DIR = 4; //2
const int8_t WINCH_MOTOR_PIN_PWM = 14; //5

GMotor2<DRIVER2WIRE> MOT_Bridge(  BRIDGE_MOTOR_PIN_DIR,   BRIDGE_MOTOR_PIN_PWM);
GMotor2<DRIVER2WIRE> MOT_Trolley( TROLLEY_MOTOR_PIN_DIR,  TROLLEY_MOTOR_PIN_PWM);
GMotor2<DRIVER2WIRE> MOT_Winch(   WINCH_MOTOR_PIN_DIR,    WINCH_MOTOR_PIN_PWM);

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Started: ");
  Serial.println(WiFi.localIP());
  udp.begin(port);

  MOT_Bridge.setMinDuty(100); // мин. ШИМ
  MOT_Bridge.reverse(1);     // реверс
  MOT_Bridge.setDeadtime(1); // deadtime


  MOT_Trolley.setMinDuty(100); // мин. ШИМ
  MOT_Trolley.reverse(1);     // реверс
  MOT_Trolley.setDeadtime(1); // deadtime

  

  MOT_Winch.setMinDuty(100); // мин. ШИМ
  MOT_Winch.reverse(1);     // реверс
  MOT_Winch.setDeadtime(1); // deadtime

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", hostname); });

  AsyncElegantOTA.begin(&server); // Start AsyncElegantOTA
  server.begin();
 
}

void loop()
{
  MOT_Bridge.tick();
  MOT_Trolley.tick();
  MOT_Winch.tick();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  static uint32_t tmr0 = millis();
  if (millis() - tmr0 > 25)
  {
    tmr0 = millis();
    int psize = udp.parsePacket();
    if (psize > 0)
    {
      char buf[32];
      int len = udp.read(buf, 32);
      IPAddress newIP;

      buf[len] = '\0';      //Парсим
      GParser data(buf);
      int ints[data.amount()];
      data.parseInts(ints);
      Serial.println(buf);

      if (newIP.fromString(buf))
      {
        if (strcmp(newIP.toString().c_str(), ServIP.toString().c_str()))
        {          
          Serial.print("Server IP changed: ");
          Serial.print(ServIP);
          Serial.print(" -- >> ");
          ServIP = newIP;
        }
        Serial.println(ServIP);
      }
      else if (ints[0] == 0)
      { 
        int Trolley_speed = map(ints[2], 0, 255, -255, 255);
        int Bridge_speed = map(ints[3], 0, 255, -255, 255);
        int Winch_speed = map(ints[4], 0, 255, -255, 255);
        if (Trolley_speed==-1){
          Trolley_speed=0;
        }
        if (Bridge_speed==-1){
          Bridge_speed=0;
        }
        if (Winch_speed==-1){
          Winch_speed=0;
        }
        MOT_Trolley.setSpeed(Trolley_speed);
        MOT_Bridge.setSpeed(Bridge_speed);
        MOT_Winch.setSpeed(Winch_speed);
      }
    }
  }

  static uint32_t tmr = millis();
  if (millis() - tmr > 25)
  {
    tmr = millis();
    char buf[32] = "_";
    strncat(buf, hostname, strlen(hostname));
    udp.beginPacket(ServIP, port);
    udp.printf(buf);
    udp.endPacket();
  } 
}
