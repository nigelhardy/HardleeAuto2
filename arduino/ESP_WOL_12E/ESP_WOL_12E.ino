// rf69 demo tx rx.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging client
// with the RH_RF69 class. RH_RF69 class does not provide for addressing or
// reliability, so you should only use RH_RF69  if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example rf69_server.
// Demonstrates the use of AES encryption, setting the frequency and modem 
// configuration

#include <SPI.h>

#include <MQTT.h>
#include <WiFiManager.h>

const char *ssid = std::getenv("WIFI_SSID");
const char *password = std::getenv("WIFI_PASS");
WiFiClient net;
MQTTClient client;
int device_id = 105;
int RELAY_PIN = 16;
int LED_PIN = 15;
unsigned long long holdTill = 0;

void setup() 
{
  Serial.begin(115200);
  //while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer

  pinMode(RELAY_PIN, OUTPUT);     
  pinMode(LED_PIN, OUTPUT);     
 
  delay(10);
  WiFiManager wifiManager;
  wifiManager.setSTAStaticIPConfig(IPAddress(192,168,0,132), IPAddress(192,168,0,1), IPAddress(255,255,255,0), IPAddress(192,168,0,201)); // optional DNS 4th argument
  wifiManager.autoConnect(ssid, password);

  Serial.println("\nsetup!");
  client.begin("hardlee.mqtt", net);
  client.setKeepAlive( 90 ); // setting keep alive to 90 seconds
  client.onMessage(messageReceived);
  connect_mqtt();
}

void loop() {
  client.loop();
  if (!client.connected()) {
    connect_mqtt();
  }
  if(holdTill > millis())
  {
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
  }
  else
  {
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
  }

}
void connect_mqtt()
{   
  Serial.print("\nconnecting...");
  String client_id = "esp-wol-" + String(device_id);

  while (!client.connect(client_id.c_str(), "YOUR_USERNAME", "YOUR_PASSWORD=")) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected!");
  client.subscribe("esp_wol/" + String(device_id) + "/pwr-btn", 2);
  client.publish("esp_wol/" + String(device_id) + "/loggin", "Startup");

}

void messageReceived(String &topic, String &payload) {
  Serial.println("MessageReceived");
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

  if(strcmp(pch, "pwr-btn") == 0)
  {
    int ms_hold = payload.toInt();
    holdTill = millis() + ms_hold;
  }
  Serial.println("incoming: " + topic + " - payload = " + String(payload));

  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}
