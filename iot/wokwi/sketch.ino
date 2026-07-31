#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <cstring>

// Wi-Fi configuration for Wokwi
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";
const int WIFI_CHANNEL = 6;

// SmartHouse backend
const char* BASE_URL =
    "https://rimmed-crave-lip.ngrok-free.dev/api";

// Timing
const unsigned long HEARTBEAT_INTERVAL_MS = 30000;
const unsigned long WIFI_RETRY_INTERVAL_MS = 5000;
const unsigned long HTTP_TIMEOUT_MS = 5000;

// Represents one logical smart-home device
struct DeviceConfig {
  const char* id;
  const char* name;
  const char* type;
};

// One ESP32 manages all 10 logical devices
DeviceConfig devices[] = {
  {"light001",  "Living Room Light",   "light"},
  {"light002",  "Kitchen Light",       "light"},
  {"door001",   "Front Door",          "door"},
  {"door002",   "Back Door",           "door"},
  {"fan001",    "Bedroom Fan",         "fan"},
  {"coffee001", "Kitchen Coffee Maker","coffee_machine"},
  {"temp001",   "Living Room Temperature", "temperature_sensor"},
  {"motion001", "Hall Motion Sensor",  "motion_sensor"},
  {"alarm001",  "Home Alarm",          "alarm"},
  {"window001", "Bedroom Window",      "window"}
};

const size_t DEVICE_COUNT =
    sizeof(devices) / sizeof(devices[0]);

Preferences preferences;

unsigned long lastHeartbeatTime = 0;
unsigned long lastWiFiRetryTime = 0;

// Connect the ESP32 to Wokwi Wi-Fi
bool connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  Serial.print("Connecting to WiFi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);

  const unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startTime < 20000) {
    delay(250);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed");
    return false;
  }

  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  return true;
}

// Add the correct initial state according to device type
void addInitialState(
    const DeviceConfig& device,
    JsonObject state) {

  if (strcmp(device.type, "light") == 0) {
    state["power"] = false;
    state["brightness"] = 0;
  }

  else if (strcmp(device.type, "door") == 0) {
    state["open"] = false;
  }

  else if (strcmp(device.type, "fan") == 0) {
    state["power"] = false;
    state["speed"] = 0;
  }

  else if (strcmp(device.type, "coffee_machine") == 0) {
    state["power"] = false;
    state["brewing"] = false;
  }

  else if (strcmp(device.type, "temperature_sensor") == 0) {
    state["temperature"] = 22.0;
  }

  else if (strcmp(device.type, "motion_sensor") == 0) {
    state["motion"] = false;
  }

  else if (strcmp(device.type, "alarm") == 0) {
    state["active"] = false;
  }

  else if (strcmp(device.type, "window") == 0) {
    state["open"] = false;
  }
}

// Read a stored secret for a device
String getStoredSecret(const char* deviceId) {
  return preferences.getString(deviceId, "");
}

// Register one device if it does not already have a secret
bool registerDevice(const DeviceConfig& device) {
  String existingSecret = getStoredSecret(device.id);

  if (existingSecret.length() > 0) {
    Serial.print(device.id);
    Serial.println(": stored secret found, registration skipped");
    return true;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(device.id);
    Serial.println(": registration skipped because WiFi is unavailable");
    return false;
  }

  WiFiClientSecure secureClient;
  secureClient.setInsecure();

  HTTPClient http;

  String url = String(BASE_URL) + "/devices/register";

  if (!http.begin(secureClient, url)) {
    Serial.print(device.id);
    Serial.println(": unable to start registration request");
    return false;
  }

  http.setTimeout(HTTP_TIMEOUT_MS);
  http.addHeader("Content-Type", "application/json");

  JsonDocument requestDocument;

  requestDocument["id"] = device.id;
  requestDocument["name"] = device.name;
  requestDocument["type"] = device.type;

  JsonObject state =
      requestDocument["state"].to<JsonObject>();

  addInitialState(device, state);

  String requestBody;
  serializeJson(requestDocument, requestBody);

  Serial.print("Registering ");
  Serial.println(device.id);

  int statusCode = http.POST(requestBody);

  String responseBody;

  if (statusCode > 0) {
    responseBody = http.getString();
  }

  Serial.print("Registration status for ");
  Serial.print(device.id);
  Serial.print(": ");
  Serial.println(statusCode);

  if (statusCode < 200 || statusCode >= 300) {
    if (responseBody.length() > 0) {
      Serial.print("Server response: ");
      Serial.println(responseBody);
    }

    http.end();
    return false;
  }

  JsonDocument responseDocument;

  DeserializationError jsonError =
      deserializeJson(responseDocument, responseBody);

  if (jsonError) {
    Serial.print(device.id);
    Serial.println(": invalid registration response");
    http.end();
    return false;
  }

  String receivedSecret =
      responseDocument["secret"].as<String>();

  if (receivedSecret.length() == 0) {
    Serial.print(device.id);
    Serial.println(": registration response contained no secret");
    http.end();
    return false;
  }

  preferences.putString(device.id, receivedSecret);

  Serial.print(device.id);
  Serial.println(": registered and secret stored");

  http.end();
  return true;
}

// Register all devices that do not yet have stored secrets
void registerMissingDevices() {
  Serial.println();
  Serial.println("Checking device registrations");

  for (size_t index = 0; index < DEVICE_COUNT; index++) {
    registerDevice(devices[index]);
    delay(250);
  }
}

// Send one heartbeat
bool sendHeartbeat(const DeviceConfig& device) {
  String deviceSecret = getStoredSecret(device.id);

  if (deviceSecret.length() == 0) {
    Serial.print(device.id);
    Serial.println(": heartbeat skipped because no secret is stored");
    return false;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(device.id);
    Serial.println(": heartbeat skipped because WiFi is unavailable");
    return false;
  }

  WiFiClientSecure secureClient;
  secureClient.setInsecure();

  HTTPClient http;

  String url = String(BASE_URL) + "/devices/heartbeat";

  if (!http.begin(secureClient, url)) {
    Serial.print(device.id);
    Serial.println(": unable to start heartbeat request");
    return false;
  }

  http.setTimeout(HTTP_TIMEOUT_MS);
  http.addHeader("Content-Type", "application/json");

  JsonDocument requestDocument;

  requestDocument["id"] = device.id;
  requestDocument["secret"] = deviceSecret;

  String requestBody;
  serializeJson(requestDocument, requestBody);

  int statusCode = http.POST(requestBody);

  String responseBody;

  if (statusCode > 0) {
    responseBody = http.getString();
  }

  Serial.print("Heartbeat status for ");
  Serial.print(device.id);
  Serial.print(": ");
  Serial.println(statusCode);

  if (statusCode < 200 || statusCode >= 300) {
    if (responseBody.length() > 0) {
      Serial.print("Server response: ");
      Serial.println(responseBody);
    }

    http.end();
    return false;
  }

  http.end();
  return true;
}

// Send heartbeat for all registered devices
void sendAllHeartbeats() {
  Serial.println();
  Serial.println("Sending device heartbeats");

  for (size_t index = 0; index < DEVICE_COUNT; index++) {
    sendHeartbeat(devices[index]);
    delay(250);
  }

  Serial.println("Heartbeat cycle completed");
}

// This part cannot be completed until the backend developer
// provides the exact command and state-update endpoints.
void handleBackendCommands() {
  // TODO:
  // 1. Request commands from the backend.
  // 2. Validate the received JSON.
  // 3. Update the correct simulated device.
  // 4. Report the updated state to the backend.
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("SmartHouse IoT starting");

  preferences.begin("smarthouse", false);

  // Use only if Antoine deletes all registered IoT devices
  // and you intentionally need to register them again:
  // preferences.clear();

  if (connectToWiFi()) {
    registerMissingDevices();

    sendAllHeartbeats();
    lastHeartbeatTime = millis();
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWiFiRetryTime >=
        WIFI_RETRY_INTERVAL_MS) {

      lastWiFiRetryTime = millis();

      if (connectToWiFi()) {
        registerMissingDevices();
        sendAllHeartbeats();
        lastHeartbeatTime = millis();
      }
    }

    delay(100);
    return;
  }

  if (millis() - lastHeartbeatTime >=
      HEARTBEAT_INTERVAL_MS) {

    lastHeartbeatTime = millis();
    sendAllHeartbeats();
  }

  handleBackendCommands();

  delay(100);
}
