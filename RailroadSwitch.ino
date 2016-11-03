#include "Arduino.h"
#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"
#include "ESP8266HTTPClient.h"
#include "EEPROM.h"
#include "WS2812_Definitions.h"
#include "RailroadSwitchCrc.h"
#include "RailroadSwitchPeripheries.h"

#define WIFI_TIMEOUT                 5000u

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

ESP8266WebServer WebServer(80);
tWiFiConfiguration WiFiConfiguration;
tWiFiTarget WiFiTarget = WT_IDLE;

const char HtmlControl[] =
  "<h1>Z&eacutemans Railroad switch #50</h1>"\
  "<p><a href=\"socket1On\"><button>LEFT</button></a>&nbsp;<a href=\"socket1Off\"><button>RIGHT</button></a></p>";

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
  "  <form action=\"save\" method=\"post\">"\
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

void ShowConfig()
{
  Serial.println("*** ShowConfig()");
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

  Serial.println("*** ReadConfigHtml()");
  Html.replace("#wifi_ssid#", WiFiConfiguration.SSID);
  Html.replace("#wifi_pwd#", WiFiConfiguration.Password);
  WebServer.send(200, "text/html", Html);
}

/*
 *
 */

void WriteConfigHtml()
{
  Serial.println("*** WriteConfigHtml()");

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
  Serial.println("*** Switch()");
  Serial.print("Direction = ");
  Serial.println(Direction);
  
  if (Direction)
  {
    Switch(0u, SWITCH_DIRECTION_LEFT);
  }
  else
  {
    Switch(0u, SWITCH_DIRECTION_RIGHT);
  }
}

/*
 * \brief Initialise the webserver
 */

void InitSoftAP(void)
{
  String HtmlTemp(HtmlConfig);

  Serial.println("*** InitSoftAP()");

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

  WebServer.on("/config", [](){
    ReadConfigHtml();
  });

  WebServer.on("/save", [](){
    WriteConfigHtml();
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
      NeoPixelSet(false, COLOR_ORANGE);
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
      NeoPixelSet(true, COLOR_ORANGE);
      break;
    }
    case WS_SOFTAP_CONNECTTED:
    {
      NeoPixelSet(false, COLOR_GREEN);
      break;
    }
    case WS_AP_CONNECTTED:
    {
      NeoPixelSet(false, COLOR_BLUE);
      break;
    }
    default:
    {
      NeoPixelSet(true, COLOR_RED);
      break;
    }
  }
}

/*
 *
 */

void WiFiControl(void)
{
  static int WiFiStatus = WS_IDLE;
  static long StartTime;
  unsigned long ElapsedTime;
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
        WiFi.begin(WiFiConfiguration.SSID, WiFiConfiguration.Password);
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
          Serial.print("SoftAP Connected - IP address: ");
          Serial.println(WiFi.localIP());
          Serial.println("");
          InitSoftAP();
          WiFiStatus = WS_SOFTAP_CONNECTTED;
        }
        else
        {
          Serial.print("WiFi status: ");
          Serial.println(ElapsedTime);
          ElapsedTime = millis() - StartTime;

          if (ElapsedTime > WIFI_TIMEOUT)
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

        if (ElapsedTime > (WIFI_TIMEOUT))
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
  SwitchInit();
  /* Set up EEPROM */
  EEPROM.begin(256);
  EEPROM.get(0, WiFiConfiguration);
  ShowConfig();
  CRC = CalcCRC(0xFF, &(WiFiConfiguration.SSID[0u]), sizeof(WiFiConfiguration) - 1u);
  Serial.print("START APP: CRC = ");
  Serial.println(CRC);
  NeoPixelInit();

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

  ButtonEvent = ButtonMonitor();

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

  WiFiControl();
  SwitchControl();
}

