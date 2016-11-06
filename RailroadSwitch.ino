/*
 * $id:$
 */

#include "Arduino.h"
#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"
#include "ESP8266HTTPClient.h"
#include "EEPROM.h"
#include "WS2812_Definitions.h"
#include "RailroadSwitchHtml.h"
#include "RailroadSwitchCrc.h"
#include "RailroadSwitchPeripheries.h"

#define WIFI_TIMEOUT                 5000u
#define SHOW_STATUS_DELAY            1000u

typedef enum
{
  WS_IDLE,
  WS_SOFTAP_START,
  WS_SOFTAP_CONNECTING,
  WS_SOFTAP_CONNECTTED,
  WS_AP_START,
  WS_AP_CONNECTTED,
  WS_DISCONNECT_START,
  WS_DISCONNECTING,
  WS_FAILED,
  WS_NUMBER,
} tWiFiStatus;

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

const String WiFiStatusLabels[WS_NUMBER] =
{
  "WS_IDLE",
  "WS_SOFTAP_START",
  "WS_SOFTAP_CONNECTING",
  "WS_SOFTAP_CONNECTTED",
  "WS_AP_START",
  "WS_AP_CONNECTTED",
  "WS_DISCONNECT_START",
  "WS_DISCONNECTING",
  "WS_FAILED",
};

ESP8266WebServer WebServer(80);
tWiFiConfiguration WiFiConfiguration;
tWiFiTarget WiFiTarget = WT_IDLE;

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
  String HtmlHead(HTML_HEAD);
  String HtmlConfig(HTML_CONFIG);

  Serial.println("*** ReadConfigHtml()");
  HtmlHead.replace("#title#", HTML_TITLE_CONFIG);
  HtmlConfig.replace("#head#", HtmlHead);
  HtmlConfig.replace("#title#", HTML_TITLE_CONFIG);
  HtmlConfig.replace("#wifi_ssid#", WiFiConfiguration.SSID);
  HtmlConfig.replace("#wifi_pwd#", WiFiConfiguration.Password);
  WebServer.send(200u, "text/html", HtmlConfig);
}

/*
 *
 */

void WriteConfigHtml()
{
  String HtmlHead(HTML_HEAD);
  String HtmlSaved(HTML_SAVED);

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
  EEPROM.put(0u, WiFiConfiguration);
  EEPROM.commit();
  HtmlHead.replace("#title#", HTML_TITLE_CONFIG);
  HtmlSaved.replace("#head#", HtmlHead);
  HtmlSaved.replace("#title#", HTML_TITLE_CONFIG);
  WebServer.send(200u, "text/html", HtmlSaved);
  WiFiTarget = WT_SOFTAP;
}

/*
 *
 */

void HandleRoot(void)
{
  String HtmlHead(HTML_HEAD);
  String HtmlControl(HTML_CONTROL);
  
  HtmlHead.replace("#title#", HTML_TITLE_CONTROL);
  HtmlControl.replace("#head#", HtmlHead);
  HtmlControl.replace("#title#", HTML_TITLE_CONTROL);
  WebServer.send(200, "text/html", HtmlControl);
}

/*
 *
 */

void Switch(bool Direction)
{
  String HtmlHead(HTML_HEAD);
  String HtmlControl(HTML_CONTROL);

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

  HtmlHead.replace("#title#", HTML_TITLE_CONTROL);
  HtmlControl.replace("#head#", HtmlHead);
  HtmlControl.replace("#title#", HTML_TITLE_CONTROL);
  WebServer.send(200u, "text/html", HtmlControl);
}

/*
 * \brief Initialise the webserver
 */

void WebServerInit(void)
{
  static bool InitDone = false;

  Serial.println("*** InitSoftAP()");

  if (!InitDone)
  {
    InitDone = true;

    WebServer.on("/", [](){
      HandleRoot();
    });

    WebServer.on("/socket1On", [](){
      Switch(true);
    });

    WebServer.on("/socket1Off", [](){
      Switch(false);
    });

    WebServer.on("/config", [](){
      ReadConfigHtml();
    });

    WebServer.on("/save", [](){
      WriteConfigHtml();
    });

    WebServer.begin();
  }
}

/*
 *
 */

void ShowStatus(unsigned int WiFiStatus)
{
  static unsigned long DisplayTime = 0u;
  static unsigned int WiFiStatusRef = WS_NUMBER;

  if ((millis() - DisplayTime) > SHOW_STATUS_DELAY)
  {
    DisplayTime = millis();
    Serial.print("WiFi status: ");
    Serial.println(WiFiStatusLabels[WiFiStatus]);
  }

  if (WiFiStatusRef != WiFiStatus)
  {
    WiFiStatusRef = WiFiStatus;
    
    switch (WiFiStatus)
    {
      case WS_IDLE:
      {
        NeoPixelSet(false, COLOR_ORANGE);
        break;
      }
      case WS_SOFTAP_START:
      case WS_SOFTAP_CONNECTING:
      case WS_AP_START:
      case WS_DISCONNECT_START:
      case WS_DISCONNECTING:
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
      case WS_FAILED:
      default:
      {
        NeoPixelSet(true, COLOR_RED);
        break;
      }
    }
  }
}

/*
 *
 */

void WiFiControl(void)
{
  static unsigned int WiFiStatus = WS_IDLE;
  static unsigned long StartTime;
  unsigned long ElapsedTime;

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
          WebServerInit();
          WiFiStatus = WS_SOFTAP_CONNECTTED;
        }
        else
        {
          ElapsedTime = millis() - StartTime;
          Serial.print("WiFi status: ");
          Serial.println(ElapsedTime);

          if (ElapsedTime > WIFI_TIMEOUT)
          {
            Serial.println("WiFi status: Unable to connect");
            WiFiTarget = WT_AP;
          }
          else
          {
            delay(500);
          }
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
      if (WiFiTarget == WT_SOFTAP)
      {
        WebServer.handleClient();
      }
      else
      {
        WiFiStatus = WS_DISCONNECT_START;
      }

      break;
    }
    /* AP CONNECTION */
    case WS_AP_START:
    {
      if (WiFiTarget == WT_AP)
      {
        Serial.print("WiFi status: AP Connected - IP address: ");
        Serial.println(WiFi.softAPIP());
        WiFi.begin();
        WiFi.softAP("RailroadSwitchAP");
        WebServerInit();
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
      if (WiFiTarget == WT_AP)
      {
        WebServer.handleClient();
      }
      else
      {
        WiFiStatus = WS_DISCONNECT_START;
      }

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
          WiFiStatus = WS_FAILED;
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
  Serial.println("*** setup()");
  SwitchInit();
  NeoPixelInit();
  /* Set up EEPROM */
  EEPROM.begin(256);
  EEPROM.get(0, WiFiConfiguration);
  ShowConfig();
  CRC = CalcCRC(0xFF, &(WiFiConfiguration.SSID[0u]), sizeof(WiFiConfiguration) - 1u);

  if (WiFiConfiguration.CRC != CRC)
  {
    Serial.println("setup(): Invalid CRC");
    WiFiConfiguration.SSID[0u] = 0;
    WiFiConfiguration.Password[0u] = 0;
    WiFiConfiguration.CRC = CalcCRC(0xFFu, &(WiFiConfiguration.SSID[0u]), sizeof(WiFiConfiguration) - 1u);
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
