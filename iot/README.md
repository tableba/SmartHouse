# SmartHouse IoT

The IoT part simulates smart-home devices using Wokwi and ESP32.

## Architecture

Android application -> Backend server -> Wokwi IoT devices

## Responsibilities

- Register simulated devices with the backend
- Store and use each device secret
- Send heartbeat messages every 30 seconds
- Receive commands from the backend
- Update simulated device states
- Support at least 10 simulated devices
