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

## Current implementation

- One ESP32 manages 10 simulated smart-home devices
- Hardware components are simulated in Wokwi
- Each device has a unique ID, type, and state
- Device registration is implemented
- Device secrets are stored using ESP32 Preferences
- Heartbeat requests are sent every 30 seconds
- Wi-Fi and backend failures are handled without crashing

## Current limitation

Backend command handling is not implemented yet because the required backend endpoint is not available. It will be added after the backend is updated.
