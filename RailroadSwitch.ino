#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <Ticker.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include "WS2812_Definitions.h"
#include "RailroadSwitchCrc.h"
#include "RailroadSwitchPeripheries.h"

#define BUTTON_PIN                   14u

#define NEO_PIN                      12u
#define NEO_COUNT                    1u

#define COLOR_BLACK                  0x00000000u
#define COLOR_RED                    0x00FF0000u
#define COLOR_GREEN                  0x0000FF00u
#define COLOR_BLUE                   0x000000FFu
#define COLOR_ORANGE                 0x00FF8000u
#define COLOR_CYAN                   0x0000FFFFu

#define C_WIFI_TIMEOUT               5000u

typedef enum
{
  SWITCH_DIRECTION_LEFT,
  SWITCH_DIRECTION_RIGHT,
} tSwitchDirection;

typedef enum
{
  WS_IDLE,
  WS_SOFTAP_START,
  WS_SOFTAP_CONNECTING,
  WS_SOFTAP_CONNECTION_FAILED,
  WS_SOFTAP_CONNECTTED,
  WS_AP_START,
  WS_AP_CONNECTTED,
  WS_DISCONNECT_START,
  WS_DISCONNECTING,
  WS_DISCONNECTION_FAILED,
  WS_COUNT,
} tWiFiState;

typedef enum
{
  WT_IDLE,
  WT_SOFTAP,
  WT_AP,
} tWiFiTarget;

typedef struct
{
  char SSID[20];
  char Password[20];
  unsigned char CRC;
} tWiFiConfiguration;

const String WiFiStateText[WS_COUNT] =
{
  "WS_IDLE",
  "WS_SOFTAP_START",
  "WS_SOFTAP_CONNECTING",
  "WS_SOFTAP_CONNECTION_FAILED",
  "WS_SOFTAP_CONNECTTED",
  "WS_AP_START",
  "WS_AP_CONNECTTED",
  "WS_DISCONNECT_START",
  "WS_DISCONNECTING",
  "WS_DISCONNECTION_FAILED",
};

Adafruit_NeoPixel Led = Adafruit_NeoPixel(NEO_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
Ticker LedTicker;
ESP8266WebServer WebServer(80);
tWiFiConfiguration WiFiConfiguration;
tWiFiTarget WiFiTarget = WT_IDLE;
uint32_t PixelColor = COLOR_BLACK;
bool PixelBlink = false;

const char HtmlControl[] =
"<h1>Z&eacutemans Railroad switch #50</h1>"\
"<p>Railroad switch <a href=\"socket1On\"><button>LEFT</button></a>&nbsp;<a href=\"socket1Off\"><button>RIGHT</button></a></p>";

const char HtmlConfig[] =
"<html>"\
"<head>"\
"   <meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">"\
"   <meta name=\"viewport\" content=\"width=device-width, user-scalable=no\">"\
"   <title>Test input text in table</title>"\
"   <style type=\"text/css\">      "\
"      input {width: 100%; height: 30px; font-family: Sans-serif; font-size: 1em}"\
"      button {width: 100%; height: 50px; font-family: Sans-serif; font-size: 1em}"\
"      h2 {font-family: Sans-serif}"\
"      body {font-family: Sans-serif}"\
"   </style>"\
"</head>"\
"<body>"\
"<h2>WiFi configuration</h2>"\
"<p>You can set WiFi connection</p>"\
"  <form action=\"config\" method=\"post\">"\
"      <b>WiFi SSID:</b>"\
"      <input type=\"text\" name=\"wifi_ssid\" value=\"#wifi_ssid#\">"\
"      <p></p>"\
"      <b>WiFi Password:</b>"\
"      <input type=\"password\" name=\"wifi_pwd\" value=\"#wifi_pwd#\">"\
"      <p></p>"\
"      <button type=\"submit\">Save</button>"\
"  </form>"\
"</body>"\
"</html>";

const char HtmlSaved[] =
"<html>"\
"<head>"\
"   <meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">"\
"   <meta name=\"viewport\" content=\"width=device-width, user-scalable=no\">"\
"   <title>Test input text in table</title>"\
"   <style type=\"text/css\">      "\
"      h2 {font-family: Sans-serif}"\
"      body {font-family: Sans-serif}"\
"   </style>"\
"</head>"\
"<body>"\
"<h2>Configuration is saved</h2>"\
"</body>"\
"</html>";

/*
 * 
 */

void LedTickerControl(void)
{
  static bool TickerState = false;

  if ((TickerState) || (!PixelBlink))
  {
    Led.setPixelColor(0, PixelColor);
  }
  else
  {
    Led.setPixelColor(0, COLOR_BLACK);
  }

  Led.show();
  TickerState = !TickerState;
}

/*
 * 
 */

void RGBLed(bool Blink, uint32_t Color)
{
  PixelBlink = Blink;
  PixelColor = Color;
}

/*
 * 
 */

void ShowConfig()
{
  Serial.println("Pedal configuration:");
  Serial.println("-------------------");
  Serial.print("WiFi SSID: ");
  Serial.println(WiFiConfiguration.SSID);
  Serial.print("WiFi Password: ");
  Serial.println(WiFiConfiguration.Password);
}

/*
 * 
 */

void ReadConfigHtml()
{
  String Html(HtmlConfig);

  Serial.println("ReadConfigHtml");
  Serial.println("--------------");
  Html.replace("#wifi_ssid#", WiFiConfiguration.SSID);
  Html.replace("#wifi_pwd#", WiFiConfiguration.Password);
  WebServer.send(200, "text/html", Html);
}

/*
 * 
 */

void WriteConfigHtml()
{
  if (WebServer.hasArg("wifi_ssid"))
  {
    strcpy(WiFiConfiguration.SSID, WebServer.arg("wifi_ssid").c_str());
  }
  
  if (WebServer.hasArg("wifi_pwd"))
  {
    strcpy(WiFiConfiguration.Password, WebServer.arg("wifi_pwd").c_str());
  }

  ShowConfig();
  WiFiConfiguration.CRC = CalcCRC(0xFF, &(WiFiConfiguration.SSID[0u]), sizeof(WiFiConfiguration) - 1u);
  EEPROM.put(0, WiFiConfiguration);
  EEPROM.commit();
  WebServer.send(200, "text/html", HtmlSaved);
}

/*
 * 
 */

void Switch(bool Direction)
{
  static int KeepActive = 0u;
  Serial.println(Direction);
}

/*
 * \brief Initialise the webserver
 */

void InitSoftAP(void)
{
  Serial.print("WiFi status: SoftAP Connected - IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");
  
  WebServer.on("/", [](){
    WebServer.send(200, "text/html", HtmlControl);
  });
  
  WebServer.on("/socket1On", [](){
    WebServer.send(200, "text/html", HtmlControl);
    Switch(true);
    delay(200);
  });
  
  WebServer.on("/socket1Off", [](){
    WebServer.send(200, "text/html", HtmlControl);
    Switch(false);
    delay(200); 
  });
  
  WebServer.on("/socket2On", [](){
    WebServer.send(200, "text/html", HtmlControl);
    Switch(true);
    delay(200);
  });
  
  WebServer.on("/socket2Off", [](){
    WebServer.send(200, "text/html", HtmlControl);
    Switch(false);
    delay(200); 
  });
  
  WebServer.begin();      
}

/*
 * 
 */

void ShowStatus(int WiFiStatus)
{
  static long DisplayTime = 0u;
  
  if ((millis() - DisplayTime) > 1000u)
  {
    DisplayTime = millis();
    Serial.print("WiFi status: ");
    Serial.println(WiFiStateText[WiFiStatus]);
  }

  switch (WiFiStatus)
  {
    case WS_IDLE:
    {
      RGBLed(false, COLOR_ORANGE);
      break;
    }
    case WS_SOFTAP_START:
    case WS_SOFTAP_CONNECTING:
    case WS_SOFTAP_CONNECTION_FAILED:
    case WS_AP_START:
    case WS_DISCONNECT_START:
    case WS_DISCONNECTING:
    case WS_DISCONNECTION_FAILED:
    {
      RGBLed(true, COLOR_RED);
      break;
    }
    case WS_SOFTAP_CONNECTTED:
    {
      RGBLed(false, COLOR_GREEN);
      break;
    }
    case WS_AP_CONNECTTED:
    {
      RGBLed(false, COLOR_BLUE);
      break;
    }
    default:
    {
      RGBLed(true, COLOR_RED);
      break;
    }
  }
}

/*
 * 
 */

void WiFiLoop(void)
{
  static int WiFiStatus = WS_IDLE;
  static long StartTime;
  long ElapsedTime;
  String Html(HtmlConfig);

  switch (WiFiStatus)
  {
    case WS_IDLE:
    {
      switch (WiFiTarget)
      {
        case WT_SOFTAP:
        {
          WiFiStatus = WS_SOFTAP_START;
          break;
        }
        case WT_AP:
        {
          WiFiStatus = WS_AP_START;
          break;
        }
        case WT_IDLE:
        default:
        {
          WiFiStatus = WS_DISCONNECT_START;
          break;
        }
      }

      break;
    }
    case WS_SOFTAP_START:
    {
      if (WiFiTarget == WT_SOFTAP)
      {
        Serial.println("WiFi status: Connect start");
        WiFi.begin("blurryext", ".Bulcsu2001");
        StartTime = millis();
        WiFiStatus = WS_SOFTAP_CONNECTING;
      }
      else
      {
        WiFiStatus = WS_DISCONNECT_START;
      }

      break;
    }
    case WS_SOFTAP_CONNECTING:
    {
      if (WiFiTarget == WT_SOFTAP)
      {
        if (WiFi.status() == WL_CONNECTED)
        {
          InitSoftAP();
          WiFiStatus = WS_SOFTAP_CONNECTTED;
        }
        else
        {
          Serial.print("WiFi status: ");
          Serial.println(ElapsedTime);
          ElapsedTime = millis() - StartTime;
          
          if (ElapsedTime > C_WIFI_TIMEOUT)
          {
            Serial.println("WiFi status: Unable to connect");
            WiFiStatus = WS_SOFTAP_CONNECTION_FAILED;
          }

          delay(500);
        }
      }
      else
      {
        WiFiStatus = WS_DISCONNECT_START;
      }

      break;
    }
    case WS_SOFTAP_CONNECTTED:
    {
      if (WiFiTarget != WT_SOFTAP)
      {
        Serial.println("WiFi status: Disconnect start");
        WiFiStatus = WS_DISCONNECT_START;
      }
      
      WebServer.handleClient();
      break;
    }
    /* AP CONNECTION */
    case WS_AP_START:
    {
      if (WiFiTarget == WT_AP)
      {
        Serial.println("WiFi status: Connect start");
        Serial.print("WiFi status: AP Connected - IP address: ");
        Serial.println(WiFi.softAPIP());
        Serial.println("------------------");
        WiFi.begin();
        WiFi.softAP("RailroadSwitchAP");
        WebServer.on("/config", ReadConfigHtml);
        WebServer.begin();
        WiFiStatus = WS_AP_CONNECTTED;
      }
      else
      {
        WiFiStatus = WS_DISCONNECT_START;
      }

      break;
    }
    case WS_AP_CONNECTTED:
    {
      if (WiFiTarget != WT_AP)
      {
        Serial.println("WiFi status: Disconnect start");
        WiFiStatus = WS_DISCONNECT_START;
      }
      
      WebServer.handleClient();
      break;
    }
    /* DISCONNECT */
    case WS_DISCONNECT_START:
    {
      Serial.println("WiFi status: Disconnecting");
      WiFi.disconnect();
      StartTime = millis();
      WiFiStatus = WS_DISCONNECTING;
      break;
    }
    case WS_DISCONNECTING:
    {
      if (WiFi.status() == WL_IDLE_STATUS)
      {
        WebServer.close();
        WiFiStatus = WS_IDLE;
      }
      else
      {
        Serial.print("WiFi status (elapsed time): ");
        Serial.println(ElapsedTime);
        ElapsedTime = millis() - StartTime;

        if (ElapsedTime > (C_WIFI_TIMEOUT))
        {
          Serial.println("WiFi status: Unable to disconnect");
          WiFiStatus = WS_DISCONNECTION_FAILED;
        }

        delay(500);
      }

      break;
    }
    default:
    {
      break;
    }
  }

  ShowStatus(WiFiStatus);
}

/*
 * 
 */

void setup()
{
  unsigned char CRC;

  Serial.begin(115200);
  Serial.println("");
  Serial.println("START APP");
  pinMode(BUTTON_PIN, INPUT);
  /* Set up LED as blue */
  Led.begin();
  /* Set up EEPROM */
  EEPROM.begin(256);
  EEPROM.get(0, WiFiConfiguration);
  ShowConfig();
  CRC = CalcCRC(0xFF, &(WiFiConfiguration.SSID[0u]), sizeof(WiFiConfiguration) - 1u);
  Serial.print("START APP: CRC = ");
  Serial.println(CRC);
  LedTicker.attach(0.25, LedTickerControl);
  RGBLed(true, COLOR_RED);

  if (WiFiConfiguration.CRC != CRC)
  {
    Serial.println("START APP: Invalid CRC");
    WiFiConfiguration.SSID[0] = 0;
    WiFiConfiguration.Password[0] = 0;
    WiFiConfiguration.CRC = CalcCRC(0xFF, &(WiFiConfiguration.SSID[0u]), sizeof(WiFiConfiguration) - 1u);
    ShowConfig();
    EEPROM.put(0u, WiFiConfiguration);
    EEPROM.commit();
    WiFiTarget = WT_AP;
  }
  else
  {
    WiFiTarget = WT_SOFTAP;
  }
}

/*
 * 
 */

void loop()
{
  tButtonEvent ButtonEvent;

  ButtonEvent = CheckButtonEvent(BUTTON_PIN);

  if (ButtonEvent == BUTTON_EVENT_LONGCLICK)
  {
    WiFiTarget = WT_AP;
  }
  else
  {
    if (ButtonEvent == BUTTON_EVENT_SHORTCLICK)
    {
      WiFiTarget = WT_SOFTAP;
    }
  }

  WiFiLoop();
}

