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
unsigned long long lastLog = 0;
bool testOnOff = false;

RCSwitch mySwitch = RCSwitch();

void setup() {
  Serial.begin ( 115200 );
  mySwitch.enableReceive(4); // NOT RELEVANT FOR ESP32 Interrupt 0 - D3/GPIO-0 on ESP12E

  delay(10);
  WiFiManager wifiManager;
  wifiManager.setHostname("ESP-RF433-Receiver");
  wifiManager.setSTAStaticIPConfig(IPAddress(192,168,0,115), IPAddress(192,168,0,1), IPAddress(255,255,255,0), IPAddress(192,168,0,201)); // optional DNS 4th argument
  wifiManager.autoConnect(ssid, password);
  Serial.println("\nsetup!");
  client.begin("hardlee.mqtt", net);
  client.onMessage(messageReceived);
}

void loop() {
  if (!client.connected()) {
    connect_mqtt();
  }
  client.loop();
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

    client.publish("rf433rx/" + String(device_id) + "/recv-payload", String(val));
  }
  if(lastLog + 60000 < millis())
  {
    lastLog = millis();
    testOnOff = !testOnOff;
    client.publish("rf433rx/" + String(device_id) + "/loggin", "Still Alive!");
  }
    
}

void connect_mqtt()
{   
  Serial.print("\nconnecting...");
  String client_id = "rf433rx-" + String(device_id);

  while (!client.connect(client_id.c_str(), "nigel", "O6XNCkmwRSGqwWXav80=")) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected!");
  client.publish("rf433rx/" + String(device_id) + "/logging", "Startup");
}

void messageReceived(String &topic, String &payload) {

}