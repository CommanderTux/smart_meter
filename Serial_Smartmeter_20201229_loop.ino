/*
  serial_smartmeter.ino - Main file for reading smartmeter

  Copyright (C) 2020  CommanderTux

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#include <ArduinoOTA.h>
#include <InfluxDb.h>
#include "Hosts.h"
#include "Functions.h"

// New version for reading smartmeter using a ESP32

#define VERSION "20201229"                                            // Version number based on date

unsigned long previousMillis = 0;                                     // will store last time
const unsigned long interval = 5UL*60UL*1000UL;                       // Schedule time 5 minutes

void setup() {
  pinMode (LED_BUILTIN, OUTPUT);                                      // Onboard LED
  Serial.begin(115200);                                               // U0UXD serial port 115k2
  Serial.println("----------------------------------------------");
  Serial.println("Version:" + String(VERSION));
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("\nConnecting to WIFI");
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
  
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  process();                                                           // execute process once before looping
  Serial.println("Wait 5 minutes...");
}

void loop() {
  ArduinoOTA.handle();
  unsigned long currentMillis = millis();                              // update with every loop
  if (currentMillis - previousMillis >= interval) {                    // Compare the time 
    previousMillis += interval;                                        // Save the time
    process();                                                         // execute process every 5 minutes
    Serial.println("Wait 5 minutes...");
  }
}
