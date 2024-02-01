/*
  Based on the SendDemo example from the RC Switch library
  https://github.com/sui77/rc-switch/
*/
#include <MQTT.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

const char *ssid = std::getenv("WIFI_SSID");
const char *password = std::getenv("WIFI_PASS");
WiFiClient net;
MQTTClient client;

#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

const int RF_SEND_PIN = D2;
long rf_payload = -1;
int device_id = 101;

void connect_mqtt()
{   
  Serial.print("\nconnecting...");
  String client_id = "rf433tx-" + String(device_id);

  while (!client.connect(client_id.c_str(), "nigel", "O6XNCkmwRSGqwWXav80=")) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected!");
  client.subscribe("rf433tx/" + String(device_id) + "/send-payload", 2);

  client.publish("rf433tx/" + String(device_id) + "/logging", "Startup");

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

  if(strcmp(pch, "send-payload") == 0)
  {
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload.c_str());
        if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      rf_payload = doc["rf_payload"];
  }

  Serial.println("incoming: " + topic + " - rf payload = " + String(rf_payload));

  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}

void setup() {
  Serial.begin ( 115200 );
  delay(10);
  WiFiManager wifiManager;
  wifiManager.setSTAStaticIPConfig(IPAddress(192,168,0,102), IPAddress(192,168,0,1), IPAddress(255,255,255,0), IPAddress(192,168,0,201)); // optional DNS 4th argument
  wifiManager.autoConnect(ssid, password);

  client.begin("hardlee.mqtt", net);
  client.onMessage(messageReceived);
  
  // Transmitter is connected to D2 (GPIO 4)  
  mySwitch.enableTransmit(RF_SEND_PIN);

  mySwitch.setProtocol(1);

  // Optional set pulse length.
  mySwitch.setPulseLength(185);
  
  // Optional set protocol (default is 1, will work for most outlets)
  mySwitch.setRepeatTransmit(15);
  
  connect_mqtt();
}

void loop() {
  if (!client.connected()) {
    connect_mqtt();
  }
  client.loop();
  if(rf_payload != -1)
  {
    mySwitch.send(rf_payload, 24);
    rf_payload = -1;
    client.publish("rf433tx/" + String(device_id) + "/send-ack", String(rf_payload));  
    delay(10);
  }
}
