#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#include <stdint.h>

// Copy to your project folder and fill in your credentials
// See arduino/README.md for complete setup instructions
const char* MQTT_USERNAME = "your_username";
const char* MQTT_PASSWORD = "your_password";

// LoRa RFM69 encryption key (16 bytes)
// Used by GarageESP32-TX-RFM69 and Garage_32u4_Feather_RX
uint8_t LORA_ENCRYPTION_KEY[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                      0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10 };

#endif