/*
 *
 */

#ifndef _RAILROAD_SWITCH_HTML_H_
#define _RAILROAD_SWITCH_HTML_H_

#define HTML_TITLE_CONFIG "WiFi configuration"
#define HTML_TITLE_CONTROL "Railroad switch"
#define HTML_URI_LEFT "/socket1left"
#define HTML_URI_RIGHT "/socket1right"

#define HTML_HEAD \
  "<head>" \
  "  <title>#title#</title>" \
  "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">" \
  "  <style type=\"text/css\">" \
  "    body {font-family: Sans-serif}" \
  "    label.fix {display :inline-block; float: clear; padding-top: 5px; text-align: right; width: 140px;}" \
  "    input {position: relative; width: 200px; height: 30px; font-family: Sans-serif; font-size: 1em}" \
  "    button {width: 100px; height: 30px; font-family: Sans-serif; font-size: 1em}" \
  "  </style>" \
  "</head>"

#define HTML_CONFIG \
  "<html>" \
  "#head#" \
  "<body>" \
  "  <h2>#title#</h2>" \
  "  <form action=\"save\" method=\"post\">" \
  "    <p>" \
  "      <label class=\"fix\" for=ssid>WiFi SSID:</label>" \
  "      <input type=\"text\" name=\"wifi_ssid\" id=\"ssid\" value=\"#wifi_ssid#\">" \
  "    </p>" \
  "    <p>" \
  "      <label class=\"fix\" for=password>WiFi Password:</label>" \
  "      <input type=\"password\" name=\"wifi_pwd\" id=\"password\"value=\"#wifi_pwd#\">" \
  "    </p>" \
  "    <br>" \
  "    <button type=\"submit\">Save</button>" \
  "  </form>" \
  "</body>" \
  "</html>"

#define HTML_SAVED \
  "<html>"\
  "#head#" \
  "<body>" \
  "  <h2>#title#</h2>" \
  "  <form action=\"\" method=\"post\">" \
  "    <p>Configuration is saved</p>" \
  "    <br>" \
  "    <button type=\"submit\">Ok</button>" \
  "  </form>" \
  "</body>" \
  "</html>"

#define HTML_CONTROL \
  "<html>" \
  "#head#" \
  "<body>" \
  "  <h2>#title#</h2>" \
  "  <p><a href=\"socket1left\"><button>LEFT</button></a>&nbsp;<a href=\"socket1right\"><button>RIGHT</button></a></p>" \
  "</body>" \
  "</html>"

#endif

