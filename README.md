# Silvia PID Controller

ESP32-based PID temperature controller for Rancilio Silvia espresso machines. Replaces the stock on/off thermostat with precise PID control for consistent brew temperatures.

For the full build tutorial, see: [PID Controlled Thermostat Using ESP32 Applied to a Rancilio Silvia Coffee Machine](https://www.instructables.com/PID-Controlled-Thermostat-Using-ESP32-Applied-to-a/)

## Hardware

- NodeMCU ESP32
- PT100 temperature sensor + MAX31865 amplifier
- 40A Solid State Relay (SSR)
- SSD1306 OLED display (128x64, I2C) - optional

## Pin Configuration

| Function | GPIO Pin |
|----------|----------|
| SSR Control | 19 |
| MAX31865 CS | 13 |
| MAX31865 DI | 14 |
| MAX31865 DO | 27 |
| MAX31865 CLK | 26 |
| OLED SDA | 21 (I2C default) |
| OLED SCL | 22 (I2C default) |

## Project Structure

```
include/
  constants.h      - PID tuning defaults
  globals.h        - Shared variable declarations
  pins.h           - Hardware pin definitions
  state_machine.h  - Machine state enum and transitions

src/
  main.cpp         - Setup, main loop, PID control
  state_machine.cpp - Machine state logic
  sensor.cpp       - Temperature reading (MAX31865)
  heater.cpp       - SSR PWM control
  screen.cpp       - OLED display
  web.cpp          - WiFi + REST API
  brewdetection.cpp - Brew detection algorithm
  config.cpp       - Persistent configuration

data/
  index.html       - Web UI
  style.css
  script.js
```

## Building

Requires [PlatformIO](https://platformio.org/).

```bash
# Build firmware
pio run

# Upload firmware
pio run -t upload

# Upload web UI files
pio run -t uploadfs
```

## Features

- Adaptive PID tuning for different machine states (cold start, ready, brewing, steam)
- Automatic brew detection
- Web interface for monitoring and configuration
- OTA firmware updates
- OLED display showing current/target temperature
