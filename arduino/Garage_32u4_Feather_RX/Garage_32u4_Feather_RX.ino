// -*- mode: C++ -*-

#include <SPI.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>

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

/************ Radio Setup ***************/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 915.0

// who am i? (server address)
#define MY_ADDRESS     1
#define DEST_ADDRESS   2


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

const int REED_SWITCH_INPUT_PIN         = 19; // A1
const int RELAY_GARAGE_DOOR_REMOTE_PIN  = 20; // A2
const int RELAY_HOLD_TIME_MS            = 1000;
const int OPEN_CLOSE_TIMEOUT            = 30000;
// variables
bool reedGarageDoorClosed       = false;
int reedSwitchState             = LOW;  // variable for reading the pushbutton status
int lastReedSwitchState         = HIGH;  // variable for reading the pushbutton status
unsigned long clickerStartTime  = 0;
bool clickerRelayActive         = false;
int16_t packetnum               = 0;  // packet counter, we increment per xmission
unsigned long pendingOpenCloseTime = 0;
unsigned long lastDebounceTime  = 0;  // the last time the output pin was toggled
unsigned long debounceDelay     = 500;    // the debounce time; increase if the output flickers

enum GARAGE_STATUS {
  UNKNOWN = 0,
  GARAGE_OPEN,
  GARAGE_CLOSED_OPENING,
  GARAGE_OPEN_CLOSING,
  GARAGE_CLOSED
};
GARAGE_STATUS garageStatus = UNKNOWN;

void setup() 
{
  Serial.begin(115200);
  //while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer
  
  // initialize the pushbutton pin as an input:
  pinMode(REED_SWITCH_INPUT_PIN, INPUT);
  delay(10);
  reedSwitchState = digitalRead(REED_SWITCH_INPUT_PIN);
  reedGarageDoorClosed = (reedSwitchState == HIGH);
  if(reedSwitchState == HIGH)
  {
    garageStatus = GARAGE_CLOSED;
  }
  else
  {
    garageStatus = GARAGE_OPEN;
  }
  pinMode(RELAY_GARAGE_DOOR_REMOTE_PIN, OUTPUT);
  digitalWrite(RELAY_GARAGE_DOOR_REMOTE_PIN, LOW);
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("Feather 32u4 Garage RFM69 LoRa!");
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
  digitalWrite(LED, HIGH);

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");
}

// Dont put this on the stack:
uint8_t data[20];
// Dont put this on the stack:
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];

void loop() {
  // read the state of the pushbutton value:
  reedSwitchState = digitalRead(REED_SWITCH_INPUT_PIN);
  if(lastReedSwitchState != reedSwitchState)
  {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:
    
    // if the button state has changed:
    if (reedGarageDoorClosed != (reedSwitchState == HIGH)) {
      reedGarageDoorClosed = (reedSwitchState == HIGH);
      HandleReedSensorChange();
    }
  }
  lastReedSwitchState = reedSwitchState;

  // control relay and activate garage remote
  if(clickerRelayActive && clickerStartTime + RELAY_HOLD_TIME_MS > millis())
  {
    digitalWrite(RELAY_GARAGE_DOOR_REMOTE_PIN, HIGH);
  }
  else if(clickerRelayActive) // done pressing garage remote button
  {
    digitalWrite(RELAY_GARAGE_DOOR_REMOTE_PIN, LOW);
    clickerRelayActive = false;
  }

  if((garageStatus == GARAGE_OPEN_CLOSING || garageStatus == GARAGE_CLOSED_OPENING) &&
    pendingOpenCloseTime + OPEN_CLOSE_TIMEOUT < millis())
  {
    Serial.println("Open/Close Timeout ERROR.");

    // handle timeout of open or close command
    if(garageStatus == GARAGE_OPEN_CLOSING && !reedGarageDoorClosed)
    {
      garageStatus = GARAGE_OPEN;
      strcpy(data, "cf"); // close failed
    }
    else if(garageStatus == GARAGE_CLOSED_OPENING && reedGarageDoorClosed)
    {
      garageStatus = GARAGE_CLOSED;
      strcpy(data, "of"); // open failed
    }
    else
    {
      Serial.println("Weird mode timeout.");
      strcpy(data, "tf"); // general timeout failure if other conditions aren't met (but always should be)
    }
    // send error to ESP/MQTT
    if (!rf69_manager.sendtoWait(data, sizeof(data), DEST_ADDRESS))
    {
      Serial.println("Sending error failed (no ack)");
    }
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
      memset(data, 0, sizeof(data));
      if((char)buf[0] == 'c')
      {
        CloseGarage();
      }
      else if((char)buf[0] == 'o')
      {
        OpenGarage();
      }
      else
      {
        SetGarageStatus();
      }
      // Send a reply back to the originator client
      if (!rf69_manager.sendtoWait(data, sizeof(data), from))
        Serial.println("Sending response failed (no ack)");
    }
  }
}

void HandleReedSensorChange()
{
  Serial.print("GarageClosed = ");
  Serial.println(reedGarageDoorClosed);
  digitalWrite(LED, reedGarageDoorClosed ? HIGH : LOW);

  if(garageStatus == GARAGE_CLOSED_OPENING && !reedGarageDoorClosed)
  {
    garageStatus = GARAGE_OPEN;
  }
  else if(garageStatus == GARAGE_OPEN_CLOSING && reedGarageDoorClosed)
  {
    garageStatus = GARAGE_CLOSED;
  }
  else if(reedGarageDoorClosed && garageStatus == GARAGE_OPEN)
  {
    // unexpectedly closed
    garageStatus = GARAGE_CLOSED;
  }
  
  
  if(!reedGarageDoorClosed && garageStatus == GARAGE_CLOSED)
  {
    // unexpectedly open
    garageStatus = GARAGE_OPEN;
    strcpy(data, "uo"); // unexpectedly opened
  }
  else
  {
    SetGarageStatus();
  }
  // Send a reply back to the originator client
  if (!rf69_manager.sendtoWait(data, sizeof(data), DEST_ADDRESS))
    Serial.println("Sending new status failed (no ack)");
}
void OpenGarage()
{
  Serial.println("Open Garage Cmd");
  // the fuzzy edge cases aren't perfect, but should work fine
  if(reedGarageDoorClosed)
  {
    garageStatus = GARAGE_CLOSED_OPENING;
    clickerStartTime = millis();
    clickerRelayActive = true;
    pendingOpenCloseTime = millis();
  }
  else if(garageStatus == GARAGE_OPEN_CLOSING)
  {
    clickerStartTime = millis();
    clickerRelayActive = true;
    pendingOpenCloseTime = millis();
    garageStatus = GARAGE_OPEN;
  }
  SetGarageStatus();
}

void CloseGarage()
{
  Serial.println("Close Garage Cmd");
  // the fuzzy edge cases aren't perfect, but should work fine
  if(!reedGarageDoorClosed)
  {
    garageStatus = GARAGE_OPEN_CLOSING;
    clickerStartTime = millis();
    clickerRelayActive = true;
    pendingOpenCloseTime = millis();
  }
  SetGarageStatus();
}

void SetGarageStatus()
{
  Serial.println("Set Garage Status for TX");
  if(garageStatus == GARAGE_CLOSED)
  {
    strcpy(data, "cc"); // closed
  }
  else if(garageStatus == GARAGE_OPEN)
  {
    strcpy(data, "oo"); // open
  }
  else if(garageStatus == GARAGE_CLOSED_OPENING)
  {
    strcpy(data, "co"); // closed but opening
  }
    else if(garageStatus == GARAGE_OPEN_CLOSING)
  {
    strcpy(data, "oc"); // open but closing
  }
}
