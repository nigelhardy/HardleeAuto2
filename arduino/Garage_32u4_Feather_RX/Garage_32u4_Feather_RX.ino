// -*- mode: C++ -*-

#include <SPI.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>

/************ Radio Setup ***************/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 915.0

// who am i? (server address)
#define MY_ADDRESS     1

#if defined (__AVR_ATmega32U4__) // Feather 32u4 w/Radio
  #define RFM69_CS      8
  #define RFM69_INT     7
  #define RFM69_RST     4
  #define LED           13
#endif

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram rf69_manager(rf69, MY_ADDRESS);

unsigned long clickerStartTime  = 0;
bool clickerRelayActive         = false;
int16_t packetnum               = 0;  // packet counter, we increment per xmission
bool garageClosed               = false;

// constants won't change. They're used here to set pin numbers:
const int reedSwitchInputPin    = 19;
const int relayGarageDoorOpener = 20;
const int relayHoldTimeMs       = 2500;
// variables will change:
int reedSwitchState = LOW;  // variable for reading the pushbutton status

void setup() 
{
  Serial.begin(115200);
  //while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer
  
  // initialize the pushbutton pin as an input:
  pinMode(reedSwitchInputPin, INPUT);
  pinMode(relayGarageDoorOpener, OUTPUT);
  digitalWrite(relayGarageDoorOpener, LOW);
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("Feather Addressed RFM69 RX Test!");
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
  // TODO for some reason larger hex values seem to break this, not sure if rx or tx side...
  // for now it is not the default and will work okay
  uint8_t key[] = { 0x29, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x09};
  rf69.setEncryptionKey(key);
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");
}


// Dont put this on the stack:
uint8_t data[20];
// Dont put this on the stack:
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];

void loop() {
  // read the state of the pushbutton value:
  reedSwitchState = digitalRead(reedSwitchInputPin);
  if(garageClosed != (reedSwitchState == HIGH))
  {
    Serial.print("GarageClosed = ");
    Serial.println(garageClosed);
    digitalWrite(LED, garageClosed ? HIGH : LOW);
  }
  garageClosed = reedSwitchState == HIGH;
  if(clickerRelayActive && clickerStartTime + 2500 > millis())
  {
    digitalWrite(relayGarageDoorOpener, HIGH);
  }
  else if(clickerRelayActive)
  {
    digitalWrite(relayGarageDoorOpener, LOW);
    clickerRelayActive = false;
  }
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
      memset(data, 0, sizeof(data));
      if((char)buf[0] == 'c')
      {
        closeGarage();
      }
      else if((char)buf[0] == 'o')
      {
        openGarage();
      }
      else
      {
        queryGarageState();
      }
      // Send a reply back to the originator client
      if (!rf69_manager.sendtoWait(data, sizeof(data), from))
        Serial.println("Sending failed (no ack)");
    }
  }
}

void openGarage()
{
  Serial.println("Open Garage Cmd");

  if(garageClosed)
  {
    strcpy(data, "OK:Open");
    clickerStartTime = millis();
    clickerRelayActive = true;
  }
  else
  {
    strcpy(data, "Fail:Open");    
  }
}

void closeGarage()
{
  Serial.println("Close Garage Cmd");
  if(!garageClosed)
  {
    strcpy(data, "OK:Close");
    clickerStartTime = millis();
    clickerRelayActive = true;
  }
  else
  {
    strcpy(data, "Fail:Close");    
  }
}

void queryGarageState()
{
    Serial.println("Query Garage Cmd");
  if(!garageClosed)
  {
    strcpy(data, "Open");
  }
  else
  {
    strcpy(data, "Closed");    
  }
}
