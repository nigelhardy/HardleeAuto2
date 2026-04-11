# HardleeAuto

Home automation system with Django + MQTT.

## Overview

- Django + Channels backend
- MQTT bridge to ESP devices
- Controls: Garage door, RGB lights, RF outlets, Shelly bulbs, WOL PCs

## Quick Start

```bash
pip install -r requirements.txt
python manage.py migrate
python manage.py runserver
```

## Hardware

- ESP8266/ESP32 for wireless devices
- RF433 modules for outlets
- RGB LED strips

## Development

```bash
# Run Django server
python manage.py runserver

# Run MQTT bridge (see Production Setup)
startbridgedjango
```

---

# Production Setup

This document covers secrets management when deploying to production hardware.

## Django Secret Key

Django requires a secret key that must NOT be committed to git.

### Generate a new key

```bash
python -c "import secrets; print(secrets.token_urlsafe(50))"
```

### Set it up

1. Copy `.env.example` to `.env` on the production server:
   ```bash
   cp .env.example .env
   ```

2. Edit `.env` and add your generated key:
   ```
   SECRET_KEY=<your-generated-key>
   ```

### If the key was accidentally committed

Use `git-filter-repo` to replace it in git history:

```bash
pip install git-filter-repo
git-filter-repo --replace-text expressions.txt
git push --force
```

See `expressions.txt` for the replacement pattern.

## MQTT Bridge Credentials

The `django_mqtt_bridge` script requires MQTT credentials. Store these securely in a separate `.env` file.

### 1. Create the credentials file

Create `~/.mqtt_bridge.env` on the server:
```
MQTT_BROKER=localhost
MQTT_PORT=1883
MQTT_USER=your_user
MQTT_PASSWORD=your_password
```

### 2. Secure the file
```bash
chmod 600 ~/.mqtt_bridge.env
```

### 3. Update your bashrc function

```bash
startbridgedjango() {
    source ~/.mqtt_bridge.env
    actdjango
    cd ~/HardleeAuto2/
    django_mqtt_bridge -H "$MQTT_BROKER" -p "$MQTT_PORT" \
        --topic rgbw-strip/#:2 --topic lora/#:2 --topic rf433rx/#:2 --topic rf433tx/#:2 \
        hardleeauto.asgi:channel_layer -U "$MQTT_USER" -P "$MQTT_PASSWORD"
}
```

### 4. Hide from bash history

Add to `.bashrc`:
```bash
export HISTCONTROL=ignorespace
```

Then run the command with a leading space: ` startbridgedjango`