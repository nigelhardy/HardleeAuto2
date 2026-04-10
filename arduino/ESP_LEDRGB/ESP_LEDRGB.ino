/*
 * Copyright (c) 2015, Majenko Technologies
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <WiFiClient.h>
#include <WiFiManager.h>
#include "credentials.h"
#include <ArduinoJson.h>
#include <MQTT.h>

#define REDPIN      16
#define GREENPIN    5
#define BLUEPIN     2
#define WHITEPIN    13

int r,g,b,w = 0;
bool is_on = false;
StaticJsonDocument<256> doc;
char out[128];
bool send_status = false;
const char *ssid = std::getenv("WIFI_SSID");
const char *password = std::getenv("WIFI_PASS");
WiFiClient net;
MQTTClient client;
int device_id = 104;

void setup ( void ) {
	Serial.begin ( 115200 );
  Serial.println("Hardlee RGBW Driver");
	Serial.println ( "" );
  delay(10);

  WiFiManager wifiManager;
  wifiManager.setSTAStaticIPConfig(IPAddress(192,168,0,100), IPAddress(192,168,0,1), IPAddress(255,255,255,0), IPAddress(192,168,0,201)); // optional DNS 4th argument
  wifiManager.autoConnect(ssid, password);
  Serial.println("\nsetup!");
  delay(10);
  client.begin("hardlee.mqtt", net);
  client.onMessage(messageReceived);

  pinMode(REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);
  pinMode(WHITEPIN, OUTPUT);
  displayColor();
}

void loop ( void ) {
  client.loop();
  if (!client.connected()) {
    connect_mqtt();
  }
  if(send_status)
  {
    publishStatus();
    send_status = false;
  }
}

void publishStatus()
{
  doc["red"] = r;
  doc["green"] = g;
  doc["blue"] = b;
  doc["is_on"] = is_on;
  int res =serializeJson(doc, out);
  client.publish("rgbw-strip/" + String(device_id) + "/status", out);
}

void connect_mqtt()
{   
  Serial.print("\nconnecting...");
  String client_id = "rgbw-strip-" + String(device_id);

  while (!client.connect(client_id.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected!");
  client.subscribe("rgbw-strip/" + String(device_id) + "/set-color", 2);
}

void messageReceived(String &topic, String &payload) {
  char topic_search[100];
  strcpy(topic_search, topic.c_str());
  char * pch;
  int cmd_depth = 2;
  pch = strtok (topic_search,"/");
  for(int i = 0; i < cmd_depth; i++)
  {
    if(pch == NULL)
      return;
    pch = strtok (NULL,"/");
  }
  if(pch == NULL)
    return;

  if(strcmp(pch, "set-color") == 0)
  {
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload.c_str());
        if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      if (doc.containsKey("is_on")) {
        is_on = doc["is_on"];
      }
      if (doc.containsKey("red")) {
        r = doc["red"];
      }
      if (doc.containsKey("green")) {
        g = doc["green"];
      }
      if (doc.containsKey("blue")) {
        b = doc["blue"];
      }
      if (doc.containsKey("white")) {
        w = doc["white"];
      }
      Serial.print("Red ");
      Serial.println(r);
      Serial.print("Green ");
      Serial.println(g);
      Serial.print("Blue ");
      Serial.println(b);
      Serial.print("White ");
      Serial.println(w);
      Serial.print("Is On: ");
      Serial.println(is_on);
      displayColor();
      send_status = true;
  }
}

void displayColor( void ) {
    // set the pins to pwm out properly
    if(is_on)
    {
      analogWrite(REDPIN, r);
      analogWrite(GREENPIN, g);
      analogWrite(BLUEPIN, b);
      analogWrite(WHITEPIN, w);
    }
    else
    {
      analogWrite(REDPIN, 0);
      analogWrite(GREENPIN, 0);
      analogWrite(BLUEPIN, 0);
      analogWrite(WHITEPIN, 0);
    }

}