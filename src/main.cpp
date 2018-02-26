/*
 * Blink
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */
#include "Arduino.h"

#include <GxEPD.h>
#include <GxGDEW042T2/GxGDEW042T2.cpp>
#include <ESP8266WiFi.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <WiFiClientSecure.h>
#include <IPAddress.h>

#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>

//#include <qrcode.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/Avenir18pt7b.h>
#include "config.h"

WiFiClientSecure client;
WiFiManager wifiManager;
WiFiManagerParameter custom_stellar_wallet("stellar_wallet", "stellar wallet address", "", 56);
bool shouldSaveConfig = false;
GxIO_Class io(SPI, D8, D3, D4);
GxEPD_Class display(io, D4, D6);

void drawWifiLogo()
{
  display.fillCircle(200, 120, 56, GxEPD_BLACK);
  display.fillCircle(200, 120, 48, GxEPD_WHITE);
  display.fillCircle(200, 120, 40, GxEPD_BLACK);
  display.fillCircle(200, 120, 32, GxEPD_WHITE);
  display.fillCircle(200, 120, 24, GxEPD_BLACK);
  display.fillCircle(200, 120, 16, GxEPD_WHITE);
  display.fillTriangle(200, 120, 80, 0, 200, 300, GxEPD_WHITE);
  display.fillTriangle(200, 120, 320, 0, 200, 300, GxEPD_WHITE);
  display.fillCircle(200, 120, 8, GxEPD_BLACK);
}

void drawHodlDeckLogo()
{
  display.fillRect(135,55, 130, 130, GxEPD_BLACK);
  display.fillCircle(200, 120, 61, GxEPD_WHITE);
  display.fillRect(177, 55, 3, 130, GxEPD_BLACK);
  display.fillRect(221, 55, 3, 130, GxEPD_BLACK);
  display.fillRect(178, 120, 43, 3, GxEPD_BLACK);
}

void printCentered(char* text, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(display.width()/2 - w/2, y);
  display.print(text);
}

void startScreenStatus(char* status)
{
  display.init();
  display.setTextColor(GxEPD_BLACK);
  display.fillScreen(GxEPD_WHITE);
  drawHodlDeckLogo();
  display.setFont(&Avenir18pt7b);
  printCentered("HODLdeck", 230);
  display.setFont(&FreeSansBold9pt7b);
  printCentered(status, 270);
  display.update();
  delay(1000);
}

void startScreen()
{
  startScreenStatus("");
}

void configModeCallback (WiFiManager *myWiFiManager) {
  display.setFont(&FreeSans9pt7b);
  display.fillScreen(GxEPD_WHITE);
  drawWifiLogo();
  printCentered("Please connect to WIFI", 170);
  display.setFont(&FreeSansBold9pt7b);
  printCentered("\"HODLDeck\"", 198);
  display.setFont(&FreeSans9pt7b);
  printCentered("and point your browser to", 226);
  display.setFont(&FreeSansBold9pt7b);
  printCentered("http://192.168.4.1/", 254);
  display.update();
}

void saveConfigCallback () {
  shouldSaveConfig = true;
}

void connectWifi()
{
  wifiManager.resetSettings();
  wifiManager.setDebugOutput(false);
  wifiManager.setAPCallback(configModeCallback);
  char address[] = "";
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_stellar_wallet);

  if (!wifiManager.autoConnect("HODLDeck")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }
}

void fetchData()
{
  display.setCursor(0, 20);
  display.setFont(&FreeSans9pt7b);
  if (!client.connect(apiHost, httpsPort)) {
     display.println("Connection Failed");
     display.update();
     return;
  }
  String url = "/api/devices";
  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
                "Host: " + apiHost + "\r\n" +
                "X-Chip-ID: " + ESP.getChipId() + "\r\n" +
                "X-Stellar-Wallet" + custom_stellar_wallet.getValue() + "\r\n" +
                "User-Agent: " + userAgent + "\r\n" +
                "Connection: close\r\n\r\n");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  String line = client.readStringUntil('\n');

  char * writable = new char[line.length() + 1];
  std::copy(line.begin(), line.end(), writable);
  writable[line.length()] = '\0';

  display.fillScreen(GxEPD_WHITE);
  printCentered("Please point your browser to", 12);
  display.setFont(&FreeSansBold9pt7b);
  printCentered("https://api.hodldeck.com/", 40);
  display.setFont(&FreeSans9pt7b);
  printCentered("and enter:", 68);
  display.setFont(&FreeSans18pt7b);
  printCentered(writable, 160);
  display.update();
}

void waitingScreen()
{
  startScreenStatus("initializing - please wait");
}

void setup()
{
  startScreen();
  connectWifi();
  waitingScreen();
  fetchData();
}

void loop()
{

}
