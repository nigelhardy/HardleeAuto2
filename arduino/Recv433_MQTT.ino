/*
  Simple example for receiving
  
  https://github.com/sui77/rc-switch/
*/

#include <RCSwitch.h>
#include <MQTT.h>
#include <WiFiManager.h>


const char *ssid = std::getenv("WIFI_SSID");
const char *password = std::getenv("WIFI_PASS");
WiFiClient net;
MQTTClient client;
int device_id = 102;

RCSwitch mySwitch = RCSwitch();

void setup() {
  Serial.begin ( 115200 );
  mySwitch.enableReceive(0); // Interrupt 0 - D3/GPIO-0 on ESP12E

  delay(10);
  WiFiManager wifiManager;
  wifiManager.setSTAStaticIPConfig(IPAddress(192,168,0,100), IPAddress(192,168,0,1), IPAddress(255,255,255,0), IPAddress(192,168,0,201)); // optional DNS 4th argument
  wifiManager.autoConnect(ssid, password);
  Serial.println("\nsetup!");
  client.begin("hardlee.mqtt", net);
  client.onMessage(messageReceived);
}

void loop() {
  if (mySwitch.available()) {
    
    Serial.print("Received ");
    unsigned long val = mySwitch.getReceivedValue();
    Serial.print(val);
    Serial.print(" / ");
    Serial.print( mySwitch.getReceivedBitlength() );
    Serial.print("bit ");
    Serial.print("Protocol: ");
    Serial.println( mySwitch.getReceivedProtocol() );

    mySwitch.resetAvailable();

    client.publish("/recv433", String(val));
  }

  if (!client.connected()) {
    connect_mqtt();
  }
    
}

void connect_mqtt()
{   
  Serial.print("\nconnecting...");
  String client_id = "rf-433recv-" + String(device_id);

  while (!client.connect(client_id.c_str(), "nigel", "O6XNCkmwRSGqwWXav80=")) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected!");
}

void messageReceived(String &topic, String &payload) {

}