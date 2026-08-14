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

## Backend simulation

Backend communication is implemented using the documented device registration, heartbeat, and device state flow.

For the Wokwi demonstration, mock backend endpoints are currently used because the Wokwi Free plan cannot access the local backend through the Private IoT Gateway.

The mock implementation simulates:

- POST /devices/register
- POST /devices/heartbeat
- GET /devices/states

This allows all 10 simulated devices to register, send heartbeats, receive desired states, and update their Wokwi hardware states.
## Wokwi simulation

https://wokwi.com/projects/471094983884003329
