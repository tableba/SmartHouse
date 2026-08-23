#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <cstring>

// --------------------------------------------------
// Wi-Fi
// --------------------------------------------------

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";
const int WIFI_CHANNEL = 6;

// --------------------------------------------------
// Backend
// --------------------------------------------------

// IMPORTANT:
// If Cloudflare Tunnel is restarted, this URL may change.
const char* BACKEND_BASE_URL =
  "https://settings-auburn-canberra-out.trycloudflare.com/api";

// --------------------------------------------------
// Timing
// --------------------------------------------------

const unsigned long HEARTBEAT_INTERVAL_MS = 30000;
const unsigned long STATE_POLL_INTERVAL_MS = 3000;
const unsigned long SENSOR_INTERVAL_MS = 2000;

// --------------------------------------------------
// Pins
// --------------------------------------------------

const int LIVING_ROOM_LIGHT_PIN = 23;
const int KITCHEN_LIGHT_PIN = 22;

const int FRONT_DOOR_SERVO_PIN = 21;
const int BACK_DOOR_SERVO_PIN = 19;

const int FAN_PIN = 18;
const int COFFEE_MACHINE_PIN = 17;

const int TEMPERATURE_SENSOR_PIN = 32;
const int MOTION_SENSOR_PIN = 33;

const int ALARM_PIN = 27;
const int WINDOW_SERVO_PIN = 26;

// --------------------------------------------------
// Hardware
// --------------------------------------------------

#define DHT_TYPE DHT22

DHT temperatureSensor(
  TEMPERATURE_SENSOR_PIN,
  DHT_TYPE
);

Servo frontDoorServo;
Servo backDoorServo;
Servo windowServo;

Preferences preferences;

// --------------------------------------------------
// Device definitions
// --------------------------------------------------

enum DeviceKind {
  LIGHT,
  DOOR,
  FAN,
  COFFEE_MACHINE,
  TEMPERATURE_SENSOR,
  MOTION_SENSOR,
  ALARM,
  WINDOW
};

struct DeviceConfig {
  const char* id;
  const char* name;
  const char* type;
  DeviceKind kind;
};

struct DeviceState {
  bool power = false;
  int brightness = 0;

  bool open = false;

  int speed = 0;

  bool brewing = false;

  float temperature = 22.0;

  bool motion = false;

  bool active = false;
};

// --------------------------------------------------
// 10 SmartHouse devices
// --------------------------------------------------

DeviceConfig devices[] = {
  {
    "light001",
    "Living Room Light",
    "light",
    LIGHT
  },
  {
    "light002",
    "Kitchen Light",
    "light",
    LIGHT
  },
  {
    "door001",
    "Front Door",
    "door",
    DOOR
  },
  {
    "door002",
    "Back Door",
    "door",
    DOOR
  },
  {
    "fan001",
    "Bedroom Fan",
    "fan",
    FAN
  },
  {
    "coffee001",
    "Kitchen Coffee Machine",
    "coffee_machine",
    COFFEE_MACHINE
  },
  {
    "temp001",
    "Living Room Temperature Sensor",
    "temperature_sensor",
    TEMPERATURE_SENSOR
  },
  {
    "motion001",
    "Hall Motion Sensor",
    "motion_sensor",
    MOTION_SENSOR
  },
  {
    "alarm001",
    "Home Alarm",
    "alarm",
    ALARM
  },
  {
    "window001",
    "Bedroom Window",
    "window",
    WINDOW
  }
};

const size_t DEVICE_COUNT =
  sizeof(devices) / sizeof(devices[0]);

DeviceState deviceStates[DEVICE_COUNT];

// --------------------------------------------------
// Timing variables
// --------------------------------------------------

unsigned long lastHeartbeatTime = 0;
unsigned long lastStatePollTime = 0;
unsigned long lastSensorReadTime = 0;

// --------------------------------------------------
// Wi-Fi
// --------------------------------------------------

bool connectToWiFi() {

  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  Serial.print("Connecting to WiFi");

  WiFi.mode(WIFI_STA);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD,
    WIFI_CHANNEL
  );

  unsigned long startTime = millis();

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - startTime < 20000
  ) {
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

// --------------------------------------------------
// Hardware setup
// --------------------------------------------------

void setupHardware() {

  pinMode(
    LIVING_ROOM_LIGHT_PIN,
    OUTPUT
  );

  pinMode(
    KITCHEN_LIGHT_PIN,
    OUTPUT
  );

  pinMode(
    FAN_PIN,
    OUTPUT
  );

  pinMode(
    COFFEE_MACHINE_PIN,
    OUTPUT
  );

  pinMode(
    MOTION_SENSOR_PIN,
    INPUT
  );

  pinMode(
    ALARM_PIN,
    OUTPUT
  );

  frontDoorServo.setPeriodHertz(50);
  backDoorServo.setPeriodHertz(50);
  windowServo.setPeriodHertz(50);

  frontDoorServo.attach(
    FRONT_DOOR_SERVO_PIN,
    500,
    2400
  );

  backDoorServo.attach(
    BACK_DOOR_SERVO_PIN,
    500,
    2400
  );

  windowServo.attach(
    WINDOW_SERVO_PIN,
    500,
    2400
  );

  temperatureSensor.begin();
}

// --------------------------------------------------
// Apply states to Wokwi hardware
// --------------------------------------------------

void applyDeviceStates() {

  digitalWrite(
    LIVING_ROOM_LIGHT_PIN,
    deviceStates[0].power
      ? HIGH
      : LOW
  );

  digitalWrite(
    KITCHEN_LIGHT_PIN,
    deviceStates[1].power
      ? HIGH
      : LOW
  );

  frontDoorServo.write(
    deviceStates[2].open
      ? 90
      : 0
  );

  backDoorServo.write(
    deviceStates[3].open
      ? 90
      : 0
  );

  digitalWrite(
    FAN_PIN,
    deviceStates[4].power
      ? HIGH
      : LOW
  );

  digitalWrite(
    COFFEE_MACHINE_PIN,
    (
      deviceStates[5].power ||
      deviceStates[5].brewing
    )
      ? HIGH
      : LOW
  );

  if (deviceStates[8].active) {

    tone(
      ALARM_PIN,
      1000
    );

  } else {

    noTone(
      ALARM_PIN
    );
  }

  windowServo.write(
    deviceStates[9].open
      ? 90
      : 0
  );
}

// --------------------------------------------------
// Read sensors
// --------------------------------------------------

void readSensors() {

  float temperature =
    temperatureSensor.readTemperature();

  if (!isnan(temperature)) {

    deviceStates[6].temperature =
      temperature;
  }

  deviceStates[7].motion =
    digitalRead(
      MOTION_SENSOR_PIN
    ) == HIGH;

  Serial.print("Temperature: ");

  Serial.print(
    deviceStates[6].temperature
  );

  Serial.print(" C, Motion: ");

  Serial.println(
    deviceStates[7].motion
      ? "true"
      : "false"
  );
}

// --------------------------------------------------
// Find device
// --------------------------------------------------

int findDeviceIndex(
  const char* deviceId
) {

  if (deviceId == nullptr) {
    return -1;
  }

  for (
    size_t i = 0;
    i < DEVICE_COUNT;
    i++
  ) {

    if (
      strcmp(
        devices[i].id,
        deviceId
      ) == 0
    ) {

      return i;
    }
  }

  return -1;
}

// --------------------------------------------------
// HTTPS helper
// --------------------------------------------------

String makeUrl(
  const char* endpoint
) {

  String url =
    BACKEND_BASE_URL;

  url += endpoint;

  return url;
}

// --------------------------------------------------
// Create registration state
// --------------------------------------------------

void createRegistrationState(
  size_t index,
  JsonObject state
) {

  switch (devices[index].kind) {

    case LIGHT:

      state["power"] =
        deviceStates[index].power;

      state["brightness"] =
        deviceStates[index].brightness;

      break;

    case DOOR:

      state["open"] =
        deviceStates[index].open;

      break;

    case FAN:

      state["power"] =
        deviceStates[index].power;

      state["speed"] =
        deviceStates[index].speed;

      break;

    case COFFEE_MACHINE:

      state["power"] =
        deviceStates[index].power;

      state["brewing"] =
        deviceStates[index].brewing;

      break;

    case TEMPERATURE_SENSOR:

      state["temperature"] =
        deviceStates[index].temperature;

      break;

    case MOTION_SENSOR:

      state["motion"] =
        deviceStates[index].motion;

      break;

    case ALARM:

      state["active"] =
        deviceStates[index].active;

      break;

    case WINDOW:

      state["open"] =
        deviceStates[index].open;

      break;
  }
}

// --------------------------------------------------
// REAL POST /devices/register
// --------------------------------------------------

bool registerDevice(
  size_t index
) {

  const DeviceConfig& device =
    devices[index];

  String existingSecret =
    preferences.getString(
      device.id,
      ""
    );

  if (
    existingSecret.length() > 0
  ) {

    Serial.print(device.id);

    Serial.println(
      ": stored secret found - registration skipped"
    );

    return true;
  }

  if (!connectToWiFi()) {
    return false;
  }

  JsonDocument document;

  document["id"] =
    device.id;

  document["name"] =
    device.name;

  document["type"] =
    device.type;

  JsonObject state =
    document["state"]
      .to<JsonObject>();

  createRegistrationState(
    index,
    state
  );

  String requestBody;

  serializeJson(
    document,
    requestBody
  );

  WiFiClientSecure client;

  // Required for Wokwi HTTPS simulation.
  client.setInsecure();

  HTTPClient http;

  String url =
    makeUrl(
      "/devices/register"
    );

  if (
    !http.begin(
      client,
      url
    )
  ) {

    Serial.println(
      "HTTP connection failed"
    );

    return false;
  }

  http.addHeader(
    "Content-Type",
    "application/json"
  );

  Serial.print(
    "POST /devices/register -> "
  );

  Serial.println(
    device.id
  );

  int statusCode =
    http.POST(
      requestBody
    );

  String response =
    http.getString();

  Serial.print(
    "Registration status: "
  );

  Serial.println(
    statusCode
  );

  if (
    statusCode == 201 ||
    statusCode == 200
  ) {

    JsonDocument responseDocument;

    DeserializationError error =
      deserializeJson(
        responseDocument,
        response
      );

    if (error) {

      Serial.println(
        "Could not parse registration response"
      );

      http.end();

      return false;
    }

    const char* secret =
      responseDocument["secret"] | "";

    if (
      strlen(secret) == 0
    ) {

      Serial.println(
        "Backend did not return a secret"
      );

      http.end();

      return false;
    }

    preferences.putString(
      device.id,
      secret
    );

    Serial.print(
      device.id
    );

    Serial.println(
      ": registered and secret stored"
    );

    http.end();

    return true;
  }

  Serial.print(
    "Registration failed: "
  );

  Serial.println(
    response
  );

  http.end();

  return false;
}

// --------------------------------------------------
// Register all missing devices
// --------------------------------------------------

void registerMissingDevices() {

  Serial.println();

  Serial.println(
    "Checking device registrations"
  );

  for (
    size_t i = 0;
    i < DEVICE_COUNT;
    i++
  ) {

    registerDevice(i);

    delay(150);
  }
}

// --------------------------------------------------
// REAL POST /devices/heartbeat
// --------------------------------------------------

bool sendHeartbeat(
  size_t index
) {

  const DeviceConfig& device =
    devices[index];

  String secret =
    preferences.getString(
      device.id,
      ""
    );

  if (
    secret.length() == 0
  ) {

    Serial.print(
      device.id
    );

    Serial.println(
      ": heartbeat skipped - no secret"
    );

    return false;
  }

  if (!connectToWiFi()) {
    return false;
  }

  JsonDocument document;

  document["id"] =
    device.id;

  document["secret"] =
    secret;

  String requestBody;

  serializeJson(
    document,
    requestBody
  );

  WiFiClientSecure client;

  client.setInsecure();

  HTTPClient http;

  String url =
    makeUrl(
      "/devices/heartbeat"
    );

  if (
    !http.begin(
      client,
      url
    )
  ) {

    return false;
  }

  http.addHeader(
    "Content-Type",
    "application/json"
  );

  int statusCode =
    http.POST(
      requestBody
    );

  String response =
    http.getString();

  Serial.print(
    "POST /devices/heartbeat -> "
  );

  Serial.print(
    device.id
  );

  Serial.print(" : ");

  Serial.println(
    statusCode
  );

  if (
    statusCode < 200 ||
    statusCode >= 300
  ) {

    Serial.print(
      "Heartbeat error: "
    );

    Serial.println(
      response
    );

    http.end();

    return false;
  }

  http.end();

  return true;
}

// --------------------------------------------------
// Send heartbeat for all devices
// --------------------------------------------------

void sendAllHeartbeats() {

  Serial.println();

  Serial.println(
    "Sending device heartbeats"
  );

  for (
    size_t i = 0;
    i < DEVICE_COUNT;
    i++
  ) {

    sendHeartbeat(i);

    delay(150);
  }

  Serial.println(
    "Heartbeat cycle completed"
  );
}

// --------------------------------------------------
// Apply desired backend state
// --------------------------------------------------

void applyDesiredState(
  size_t index,
  JsonObject state
) {

  switch (
    devices[index].kind
  ) {

    case LIGHT:

      if (
        !state["power"].isNull()
      ) {

        deviceStates[index].power =
          state["power"]
            .as<bool>();
      }

      if (
        !state["brightness"].isNull()
      ) {

        deviceStates[index].brightness =
          constrain(
            state["brightness"]
              .as<int>(),
            0,
            100
          );
      }

      break;

    case DOOR:

      if (
        !state["open"].isNull()
      ) {

        deviceStates[index].open =
          state["open"]
            .as<bool>();
      }

      break;

    case FAN:

      if (
        !state["power"].isNull()
      ) {

        deviceStates[index].power =
          state["power"]
            .as<bool>();
      }

      if (
        !state["speed"].isNull()
      ) {

        deviceStates[index].speed =
          state["speed"]
            .as<int>();
      }

      break;

    case COFFEE_MACHINE:

      if (
        !state["power"].isNull()
      ) {

        deviceStates[index].power =
          state["power"]
            .as<bool>();
      }

      if (
        !state["brewing"].isNull()
      ) {

        deviceStates[index].brewing =
          state["brewing"]
            .as<bool>();
      }

      break;

    case ALARM:

      if (
        !state["active"].isNull()
      ) {

        deviceStates[index].active =
          state["active"]
            .as<bool>();
      }

      break;

    case WINDOW:

      if (
        !state["open"].isNull()
      ) {

        deviceStates[index].open =
          state["open"]
            .as<bool>();
      }

      break;

    case TEMPERATURE_SENSOR:
    case MOTION_SENSOR:

      // These sensors are read locally.
      // Sensor reporting to backend is not implemented.
      break;
  }
}

// --------------------------------------------------
// REAL GET /devices/states
// --------------------------------------------------

bool fetchDesiredStates() {

  if (!connectToWiFi()) {
    return false;
  }

  WiFiClientSecure client;

  client.setInsecure();

  HTTPClient http;

  String url =
    makeUrl(
      "/devices/states"
    );

  if (
    !http.begin(
      client,
      url
    )
  ) {

    Serial.println(
      "Could not start GET request"
    );

    return false;
  }

  Serial.println();

  Serial.println(
    "GET /devices/states"
  );

  int statusCode =
    http.GET();

  String response =
    http.getString();

  Serial.print(
    "Device states status: "
  );

  Serial.println(
    statusCode
  );

  if (
    statusCode < 200 ||
    statusCode >= 300
  ) {

    Serial.print(
      "GET states error: "
    );

    Serial.println(
      response
    );

    http.end();

    return false;
  }

  JsonDocument document;

  DeserializationError error =
    deserializeJson(
      document,
      response
    );

  if (error) {

    Serial.print(
      "JSON error: "
    );

    Serial.println(
      error.c_str()
    );

    http.end();

    return false;
  }

  if (
    !document.is<JsonArray>()
  ) {

    Serial.println(
      "Backend response is not an array"
    );

    http.end();

    return false;
  }

  JsonArray serverDevices =
    document.as<JsonArray>();

  Serial.print(
    "Received "
  );

  Serial.print(
    serverDevices.size()
  );

  Serial.println(
    " backend devices"
  );

  for (
    JsonVariant item :
    serverDevices
  ) {

    const char* deviceId =
      item["id"] | "";

    int index =
      findDeviceIndex(
        deviceId
      );

    // Ignore backend devices that do not belong
    // to this Wokwi simulation.
    if (
      index < 0
    ) {

      continue;
    }

    JsonObject state =
      item["state"]
        .as<JsonObject>();

    applyDesiredState(
      index,
      state
    );

    Serial.print(
      "Updated desired state: "
    );

    Serial.println(
      deviceId
    );
  }

  applyDeviceStates();

  http.end();

  return true;
}

// --------------------------------------------------
// Print states
// --------------------------------------------------

void printCurrentStates() {

  Serial.println(
    "Current simulated states:"
  );

  Serial.print(
    "light001: "
  );

  Serial.println(
    deviceStates[0].power
      ? "ON"
      : "OFF"
  );

  Serial.print(
    "light002: "
  );

  Serial.println(
    deviceStates[1].power
      ? "ON"
      : "OFF"
  );

  Serial.print(
    "door001: "
  );

  Serial.println(
    deviceStates[2].open
      ? "OPEN"
      : "CLOSED"
  );

  Serial.print(
    "door002: "
  );

  Serial.println(
    deviceStates[3].open
      ? "OPEN"
      : "CLOSED"
  );

  Serial.print(
    "fan001: "
  );

  Serial.println(
    deviceStates[4].power
      ? "ON"
      : "OFF"
  );

  Serial.print(
    "coffee001: "
  );

  Serial.println(
    deviceStates[5].brewing
      ? "BREWING"
      : "OFF"
  );

  Serial.print(
    "alarm001: "
  );

  Serial.println(
    deviceStates[8].active
      ? "ACTIVE"
      : "OFF"
  );

  Serial.print(
    "window001: "
  );

  Serial.println(
    deviceStates[9].open
      ? "OPEN"
      : "CLOSED"
  );

  Serial.println();
}

// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup() {

  Serial.begin(
    115200
  );

  delay(500);

  Serial.println();

  Serial.println(
    "SmartHouse IoT starting"
  );

  Serial.println(
    "Backend mode: REAL"
  );

  Serial.print(
    "Backend URL: "
  );

  Serial.println(
    BACKEND_BASE_URL
  );

  setupHardware();

  applyDeviceStates();

  /*
   * Different namespace from the old mock backend.
   * This prevents old mock secrets from being reused.
   */
  preferences.begin(
    "smarthouse-real",
    false
  );

  /*
   * Only uncomment this if you need to erase
   * all stored REAL backend secrets.
   */
  // preferences.clear();

  delay(1000);

  readSensors();

  if (
    connectToWiFi()
  ) {

    registerMissingDevices();

    sendAllHeartbeats();

    fetchDesiredStates();
  }

  printCurrentStates();

  lastHeartbeatTime =
    millis();

  lastStatePollTime =
    millis();

  lastSensorReadTime =
    millis();
}

// --------------------------------------------------
// Loop
// --------------------------------------------------

void loop() {

  unsigned long currentTime =
    millis();

  // Sensors every 2 seconds
  if (
    currentTime -
      lastSensorReadTime >=
    SENSOR_INTERVAL_MS
  ) {

    lastSensorReadTime =
      currentTime;

    readSensors();
  }

  // Backend states every 3 seconds
  if (
    currentTime -
      lastStatePollTime >=
    STATE_POLL_INTERVAL_MS
  ) {

    lastStatePollTime =
      currentTime;

    fetchDesiredStates();

    printCurrentStates();
  }

  // Heartbeat every 30 seconds
  if (
    currentTime -
      lastHeartbeatTime >=
    HEARTBEAT_INTERVAL_MS
  ) {

    lastHeartbeatTime =
      currentTime;

    sendAllHeartbeats();
  }

  applyDeviceStates();

  delay(100);
}