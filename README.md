# TinyML Desk Activity Monitor

An ESP32-S3 based smart desk activity monitoring system using TinyML for person detection with environmental sensing.

## Project Overview

This project implements a privacy-preserving desk occupancy monitoring system using:
- ESP32-S3 WROOM microcontroller
- OV2640 camera module
- LDR light sensor
- MAX9814 microphone
- ThingSpeak cloud integration

## Features

- Real-time light level monitoring (Dark/Dim/Bright)
- Ambient noise detection (Quiet/Moderate/Loud)
- Live camera streaming via local web dashboard
- Cloud data logging to ThingSpeak (15-second intervals)
- Privacy-preserving on-device processing

## Hardware Requirements

| Component | Specification |
|-----------|---------------|
| Microcontroller | ESP32-S3 WROOM (Freenove) |
| Camera | OV2640 (built-in) |
| Light Sensor | LDR with 10kΩ resistor |
| Microphone | MAX9814 |
| LED | Standard LED with 220Ω resistor |

## Pin Configuration

| Component | GPIO Pin |
|-----------|----------|
| LDR | GPIO1 |
| Microphone | GPIO2 |
| Status LED | GPIO21 |
| Camera | Multiple (see documentation) |

## Software Requirements

- Arduino IDE 2.3.6+
- ESP32 Board Package 3.3.4
- Libraries: esp_camera.h, WiFi.h, HTTPClient.h, esp_http_server.h

## Installation

1. Clone this repository
2. Open the `.ino` file in Arduino IDE
3. Update WiFi credentials and ThingSpeak API key
4. Select "ESP32S3 Dev Module" as board
5. Upload to your ESP32-S3

## Configuration

Edit these lines in the code with your credentials:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* apiKey = "YOUR_THINGSPEAK_API_KEY";
```

## ThingSpeak Channel

Channel ID: 3200846

## Author

Tosin Akinwoye - Northumbria University
Module: LD7182 - AI for IoT

## License

This project is submitted as coursework for LD7182.
