// -*- mode: C++ -*-
/* 
 KEY for comms
 oo = open
 cc = closed
 oc = open and attempting to close
 co = closed and attempting to open
 cf = close failed
 of = open failed
 tf = general unknown timeout from open or close attempt
 uo = unexpectedly opened
*/

#include <SPI.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>

#include <MQTT.h>
#include <WiFiManager.h>

const char *ssid = std::getenv("WIFI_SSID");
const char *password = std::getenv("WIFI_PASS");
WiFiClient net;
MQTTClient client;
int device_id = 103;

unsigned long long lastCheck = 0;
/************ Radio Setup ***************/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 915.0

// Where to send packets to!
#define DEST_ADDRESS   1
// change addresses for each client board, any number :)
#define MY_ADDRESS     2

#if defined(ESP32)    // ESP32 feather w/wing
  // #define RFM69_RST     13   // same as LED
  // #define RFM69_CS      33   // "B"
  // #define RFM69_INT     27   // "A"
  // #define LED           13
  // Updated for the FeatherS3 which is ESP32 based
  #define RFM69_RST  38  // "D"
  #define RFM69_CS   33  // "E"
  #define RFM69_INT  3  //  "B"
  #define LED        13
#endif

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram rf69_manager(rf69, MY_ADDRESS);

char radiopacket[20];
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];

enum TX_COMMAND {
  NOTHING = 0,
  QUERY_GARAGE,
  OPEN_GARAGE,
  CLOSE_GARAGE
};
TX_COMMAND current_command = NOTHING;

int16_t packetnum = 0;  // packet counter, we increment per xmission
const int LOGGING_INTERNAL_MS = 60000;
void setup() 
{
  Serial.begin(115200);
  //while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer

  pinMode(LED, OUTPUT);     
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("FeatherS3 RFM69 TX Garage LoRa");
  Serial.println();

  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  
  if (!rf69_manager.init()) {
    Serial.println("RFM69 radio init failed");
    while (1);
  }
  Serial.println("RFM69 radio init OK!");
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  // No encryption
  if (!rf69.setFrequency(RF69_FREQ)) {
    Serial.println("setFrequency failed");
  }

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(20, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW

  // The encryption key has to be the same as the one in the server
  uint8_t key[] = { 0x29, 0x02, 0x21, 0x04, 0x45, 0x90, 0x37, 0x08,
                    0x75, 0x20, 0x04, 0x03, 0x05, 0x06, 0x11, 0x09};

  rf69.setEncryptionKey(key);
  
  pinMode(LED, OUTPUT);

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");

  delay(10);
  WiFiManager wifiManager;
  wifiManager.setSTAStaticIPConfig(IPAddress(192,168,0,101), IPAddress(192,168,0,1), IPAddress(255,255,255,0), IPAddress(192,168,0,201)); // optional DNS 4th argument
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
  if(lastCheck + LOGGING_INTERNAL_MS < millis())
  {
    lastCheck = millis();
    client.publish("esp_lora/" + String(device_id) + "/logging", "Still Alive!");
  }

  // recieve data from LoRa
  if (rf69_manager.available())
  {
    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (rf69_manager.recvfromAck(buf, &len, &from)) {
      buf[len] = 0; // zero out remaining string
      
      Serial.print("Got packet from #");
      Serial.print(from);
      Serial.print(" [RSSI :");
      Serial.print(rf69.lastRssi());
      Serial.print("] : ");
      Serial.println((char*)buf);
      // TODO handle different messages and send readable payloads over MQTT
      client.publish("esp_lora/" + String(device_id) + "/garage-status", String((char*)buf));
    }
  }

  if(current_command != NOTHING)
  {
    // Now wait for a reply
    uint8_t len = sizeof(buf);
    switch (current_command) {
      case OPEN_GARAGE:
        strcpy(radiopacket, "o");
        break;
      case CLOSE_GARAGE:
        strcpy(radiopacket, "c");
        break;
      case QUERY_GARAGE:
        strcpy(radiopacket, "q");
        break;
      default:
        strcpy(radiopacket, "invalid");
        Serial.println("Shouldn't have command at default state here!");
        break;
    }
    current_command = NOTHING;

    // Send a message to the DESTINATION!
    if (rf69_manager.sendtoWait((uint8_t *)radiopacket, strlen(radiopacket), DEST_ADDRESS)) {
      // Now wait for a reply from the server
      uint8_t len = sizeof(buf);
      uint8_t from;
      if (rf69_manager.recvfromAckTimeout(buf, &len, 2000, &from)) {
        buf[len] = 0; // zero out remaining string
        
        Serial.print("Got reply from #"); Serial.print(from);
        Serial.print(" [RSSI :");
        Serial.print(rf69.lastRssi());
        Serial.print("] : ");
        Serial.println((char*)buf);
        // TODO handle different messages and send readable payloads over MQTT
        client.publish("esp_lora/" + String(device_id) + "/garage-status", String((char*)buf));

      } else {
        Serial.println("No reply, is anyone listening?");
      }
    } else {
      Serial.println("Sending failed (no ack)");
    }
  }

}
void connect_mqtt()
{   
  Serial.print("\nconnecting...");
  String client_id = "rf-433-" + String(device_id);

  while (!client.connect(client_id.c_str(), "nigel", "O6XNCkmwRSGqwWXav80=")) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected!");
  client.subscribe("esp_lora/" + String(device_id) + "/open-garage", 2);
  client.subscribe("esp_lora/" + String(device_id) + "/close-garage", 2);
  client.subscribe("esp_lora/" + String(device_id) + "/query-garage", 2);
  client.publish("esp_lora/" + String(device_id) + "/loggin", "Startup");

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

  if(strcmp(pch, "open-garage") == 0)
  {
    Serial.println("Open Garage");
    current_command = OPEN_GARAGE;
  }
  if(strcmp(pch, "close-garage") == 0)
  {
    Serial.println("Close Garage");
    current_command = CLOSE_GARAGE;
  }
  if(strcmp(pch, "query-garage") == 0)
  {
    Serial.println("Query");
    current_command = QUERY_GARAGE;

  }
  Serial.println("incoming: " + topic + " - payload = " + String(payload));

  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}
