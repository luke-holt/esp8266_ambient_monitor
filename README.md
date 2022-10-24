# Description
Ambient conditions monitoring device I built using Espressif's ESP IDF to learn FreeRTOS concepts.

This app is written for the ESP8266 wifi module.

Periodically read ambient temperature, humidity, luminosity and send to MQTT broker.

## Components
- I2C driver for AM2301B
- I2C driver for LTR390
- I2C helper functions.

## Concepts
- I2C
- WiFi
- MQTT
- FreeRTOS
