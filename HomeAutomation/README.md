# HomeAutomation – IntelliGB Control Node

ESP8266-based home-automation controller with LED feedback and DHT sensors.

## Quick Start

| Command | Description |
| --- | --- |
| `make build` | Compile firmware |
| `make upload` | Build & flash to device |
| `make clean` | Remove build artifacts |
| `make monitor` | Open serial monitor |

## Prerequisites

- PlatformIO (`pip install platformio`)
- USB-serial driver for ESP8266

## Hardware

- **Board:** ESP-12E
- **LED strip:** WS2812B on GPIO 1, 8 LEDs
- **Sensors:** Up to 2× DHT11 (pin configured via server)
- **Reset trigger:** Analog pin A0 ≥ 700 → factory reset

## Network / Server

- **WebSocket:** `167.172.144.39:80` (Socket.IO v4)
- **WiFi AP:** `IntelliGB` / password `12345678`
- **Link params:** linkCode, homeId, roomId (captured via captive portal)

## Architecture

1. Boots → checks A0 for factory reset
2. Starts WiFiManager captive portal
3. On WiFi: exchanges tokens via Socket.IO `LINKED:` event
4. Receives pin config (`PINSYNC`) → configures outputs & DHT sensors
5. Every 2 s uploads temperature & humidity via `BOARD_ID:UPLOAD`
6. Listens for `SYNC` / `boardCreatedId` events → drives digital outputs
