# Arduino Projects

This folder contains various Arduino/ESP projects for home automation.

## Credentials

All projects require `credentials.h` with MQTT credentials and optionally LoRa encryption key. This file is **not** committed to git (see `.gitignore`).

### LoRa Encryption Key

For GarageESP32-TX-RFM69 and Garage_32u4_Feather_RX projects, the credentials.h template includes a LoRa RFM69 encryption key (`LORA_ENCRYPTION_KEY`). Copy the template to these projects.

### Setup

Copy the template credentials.h from this folder to your project:

```bash
# Example for WOL_ESP32_MQTT
cp credentials.h WOL_ESP32_MQTT/credentials.h
```

Then edit the credentials file in your project folder and fill in your actual values:
- Check your password manager for the MQTT credentials and LoRa encryption key
- LoRa: https://arduino-lora-encryption-key.com
- MQTT: https://mqtt-hardlee-rpi-not_a_real_ip/

### Available Projects

| Project | Description |
|---------|-------------|
| ESP_WOL_12E| Wake on LAN with ESP-01|
| WOL_ESP32_MQTT| Wake on LAN with ESP32|
| WOL-ESP-Update1-Webserver | Web-updated WOL|
| RF433-Tx-MQTT | RF 433MHz transmitter  |
| RF433-Rx-MQTT | RF 433MHz receiver |
| ESP_LEDRGB | RGB LED controller |
| GarageESP32-TX-RFM69 | Garage controller (RFM69) |

## Building

1. Install [PlatformIO](https://platformio.org/) or use Arduino IDE
2. Copy credentials.h to your project folder
3. Build and upload
