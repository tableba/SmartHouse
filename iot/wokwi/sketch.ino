#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <cstring>

// --------------------------------------------------
// Wi-Fi configuration
// --------------------------------------------------

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";
const int WIFI_CHANNEL = 6;

// The Ngrok URL may need to be changed later.
const char* BASE_URL =
    "https://rimmed-crave-lip.ngrok-free.dev/api";

// --------------------------------------------------
// Timing
// --------------------------------------------------

const unsigned long HEARTBEAT_INTERVAL_MS = 30000;
const unsigned long REGISTRATION_RETRY_INTERVAL_MS = 60000;
const unsigned long WIFI_RETRY_INTERVAL_MS = 5000;
const unsigned long SENSOR_INTERVAL_MS = 2000;
const unsigned long HTTP_TIMEOUT_MS = 8000;

// --------------------------------------------------
// ESP32 pins
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
// Sensor configuration
// --------------------------------------------------

#define DHT_TYPE DHT22

DHT temperatureSensor(TEMPERATURE_SENSOR_PIN, DHT_TYPE);

Servo frontDoorServo;
Servo backDoorServo;
Servo windowServo;

Preferences preferences;

// --------------------------------------------------
// Device models
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
  bool power;
  int brightness;

  bool open;

  int speed;

  bool brewing;

  float temperature;

  bool motion;

  bool active;
};

// One ESP32 manages all 10 logical devices.
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
// Runtime timing variables
// --------------------------------------------------

unsigned long lastHeartbeatTime = 0;
unsigned long lastRegistrationRetryTime = 0;
unsigned long lastWiFiRetryTime = 0;
unsigned long lastSensorReadTime = 0;

// --------------------------------------------------
// Initial device states
// --------------------------------------------------

void initializeDeviceStates() {
  for (size_t index = 0; index < DEVICE_COUNT; index++) {
    deviceStates[index] = DeviceState{};
  }

  // light001
  deviceStates[0].power = false;
  deviceStates[0].brightness = 0;

  // light002
  deviceStates[1].power = false;
  deviceStates[1].brightness = 0;

  // door001
  deviceStates[2].open = false;

  // door002
  deviceStates[3].open = false;

  // fan001
  deviceStates[4].power = false;
  deviceStates[4].speed = 0;

  // coffee001
  deviceStates[5].power = false;
  deviceStates[5].brewing = false;

  // temp001
  deviceStates[6].temperature = 22.0;

  // motion001
  deviceStates[7].motion = false;

  // alarm001
  deviceStates[8].active = false;

  // window001
  deviceStates[9].open = false;
}

// --------------------------------------------------
// Hardware setup and output control
// --------------------------------------------------

void setupHardware() {
  pinMode(LIVING_ROOM_LIGHT_PIN, OUTPUT);
  pinMode(KITCHEN_LIGHT_PIN, OUTPUT);

  pinMode(FAN_PIN, OUTPUT);
  pinMode(COFFEE_MACHINE_PIN, OUTPUT);

  pinMode(MOTION_SENSOR_PIN, INPUT);
  pinMode(ALARM_PIN, OUTPUT);

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

void applyDeviceStates() {
  digitalWrite(
      LIVING_ROOM_LIGHT_PIN,
      deviceStates[0].power ? HIGH : LOW
  );

  digitalWrite(
      KITCHEN_LIGHT_PIN,
      deviceStates[1].power ? HIGH : LOW
  );

  frontDoorServo.write(
      deviceStates[2].open ? 90 : 0
  );

  backDoorServo.write(
      deviceStates[3].open ? 90 : 0
  );

  digitalWrite(
      FAN_PIN,
      deviceStates[4].power ? HIGH : LOW
  );

  digitalWrite(
      COFFEE_MACHINE_PIN,
      deviceStates[5].power ||
      deviceStates[5].brewing
          ? HIGH
          : LOW
  );

  if (deviceStates[8].active) {
    tone(ALARM_PIN, 1000);
  } else {
    noTone(ALARM_PIN);
  }

  windowServo.write(
      deviceStates[9].open ? 90 : 0
  );
}

// --------------------------------------------------
// Sensor reading
// --------------------------------------------------

void readSensors() {
  float temperature =
      temperatureSensor.readTemperature();

  if (!isnan(temperature)) {
    deviceStates[6].temperature = temperature;
  }

  deviceStates[7].motion =
      digitalRead(MOTION_SENSOR_PIN) == HIGH;

  Serial.print("Temperature: ");
  Serial.print(deviceStates[6].temperature);
  Serial.print(" C, Motion: ");
  Serial.println(
      deviceStates[7].motion ? "true" : "false"
  );
}

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

  const unsigned long startTime = millis();

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
// JSON state generation
// --------------------------------------------------

void addStateToJson(
    size_t deviceIndex,
    JsonObject state
) {
  DeviceKind kind = devices[deviceIndex].kind;

  switch (kind) {
    case LIGHT:
      state["power"] =
          deviceStates[deviceIndex].power;

      state["brightness"] =
          deviceStates[deviceIndex].brightness;
      break;

    case DOOR:
      state["open"] =
          deviceStates[deviceIndex].open;
      break;

    case FAN:
      state["power"] =
          deviceStates[deviceIndex].power;

      state["speed"] =
          deviceStates[deviceIndex].speed;
      break;

    case COFFEE_MACHINE:
      state["power"] =
          deviceStates[deviceIndex].power;

      state["brewing"] =
          deviceStates[deviceIndex].brewing;
      break;

    case TEMPERATURE_SENSOR:
      state["temperature"] =
          deviceStates[deviceIndex].temperature;
      break;

    case MOTION_SENSOR:
      state["motion"] =
          deviceStates[deviceIndex].motion;
      break;

    case ALARM:
      state["active"] =
          deviceStates[deviceIndex].active;
      break;

    case WINDOW:
      state["open"] =
          deviceStates[deviceIndex].open;
      break;
  }
}

// --------------------------------------------------
// Device secret storage
// --------------------------------------------------

String getStoredSecret(const char* deviceId) {
  return preferences.getString(deviceId, "");
}

bool hasMissingDeviceSecrets() {
  for (size_t index = 0; index < DEVICE_COUNT; index++) {
    if (getStoredSecret(devices[index].id).length() == 0) {
      return true;
    }
  }

  return false;
}

// --------------------------------------------------
// Device registration
// --------------------------------------------------

bool registerDevice(size_t deviceIndex) {
  const DeviceConfig& device =
      devices[deviceIndex];

  String existingSecret =
      getStoredSecret(device.id);

  if (existingSecret.length() > 0) {
    Serial.print(device.id);
    Serial.println(
        ": stored secret found, registration skipped"
    );

    return true;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(device.id);
    Serial.println(
        ": registration skipped because WiFi is unavailable"
    );

    return false;
  }

  WiFiClientSecure secureClient;
  secureClient.setInsecure();

  HTTPClient http;

  String url =
      String(BASE_URL) + "/devices/register";

  if (!http.begin(secureClient, url)) {
    Serial.print(device.id);
    Serial.println(
        ": unable to create registration request"
    );

    return false;
  }

  http.setTimeout(HTTP_TIMEOUT_MS);
  http.addHeader(
      "Content-Type",
      "application/json"
  );

  JsonDocument requestDocument;

  requestDocument["id"] = device.id;
  requestDocument["name"] = device.name;
  requestDocument["type"] = device.type;

  JsonObject state =
      requestDocument["state"].to<JsonObject>();

  addStateToJson(deviceIndex, state);

  String requestBody;
  serializeJson(
      requestDocument,
      requestBody
  );

  Serial.print("Registering ");
  Serial.println(device.id);

  int statusCode =
      http.POST(requestBody);

  String responseBody;

  if (statusCode > 0) {
    responseBody = http.getString();
  }

  Serial.print("Registration status for ");
  Serial.print(device.id);
  Serial.print(": ");
  Serial.println(statusCode);

  if (
      statusCode < 200 ||
      statusCode >= 300
  ) {
    if (responseBody.length() > 0) {
      Serial.print("Server response: ");
      Serial.println(responseBody);
    }

    http.end();
    return false;
  }

  JsonDocument responseDocument;

  DeserializationError jsonError =
      deserializeJson(
          responseDocument,
          responseBody
      );

  if (jsonError) {
    Serial.print(device.id);
    Serial.println(
        ": invalid registration response"
    );

    http.end();
    return false;
  }

  String receivedSecret =
      responseDocument["secret"].as<String>();

  if (receivedSecret.length() == 0) {
    Serial.print(device.id);
    Serial.println(
        ": registration response contained no secret"
    );

    http.end();
    return false;
  }

  preferences.putString(
      device.id,
      receivedSecret
  );

  Serial.print(device.id);
  Serial.println(
      ": registered and secret stored"
  );

  http.end();
  return true;
}

void registerMissingDevices() {
  Serial.println();
  Serial.println("Checking device registrations");

  for (
      size_t index = 0;
      index < DEVICE_COUNT;
      index++
  ) {
    registerDevice(index);
    delay(200);
  }
}

// --------------------------------------------------
// Heartbeat
// --------------------------------------------------

bool sendHeartbeat(size_t deviceIndex) {
  const DeviceConfig& device =
      devices[deviceIndex];

  String deviceSecret =
      getStoredSecret(device.id);

  if (deviceSecret.length() == 0) {
    Serial.print(device.id);
    Serial.println(
        ": heartbeat skipped because no secret is stored"
    );

    return false;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(device.id);
    Serial.println(
        ": heartbeat skipped because WiFi is unavailable"
    );

    return false;
  }

  WiFiClientSecure secureClient;
  secureClient.setInsecure();

  HTTPClient http;

  String url =
      String(BASE_URL) + "/devices/heartbeat";

  if (!http.begin(secureClient, url)) {
    Serial.print(device.id);
    Serial.println(
        ": unable to create heartbeat request"
    );

    return false;
  }

  http.setTimeout(HTTP_TIMEOUT_MS);
  http.addHeader(
      "Content-Type",
      "application/json"
  );

  JsonDocument requestDocument;

  requestDocument["id"] = device.id;
  requestDocument["secret"] = deviceSecret;

  String requestBody;
  serializeJson(
      requestDocument,
      requestBody
  );

  int statusCode =
      http.POST(requestBody);

  String responseBody;

  if (statusCode > 0) {
    responseBody = http.getString();
  }

  Serial.print("Heartbeat status for ");
  Serial.print(device.id);
  Serial.print(": ");
  Serial.println(statusCode);

  if (
      statusCode < 200 ||
      statusCode >= 300
  ) {
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

void sendAllHeartbeats() {
  Serial.println();
  Serial.println("Sending device heartbeats");

  for (
      size_t index = 0;
      index < DEVICE_COUNT;
      index++
  ) {
    sendHeartbeat(index);
    delay(200);
  }

  Serial.println("Heartbeat cycle completed");
}

// --------------------------------------------------
// Backend commands
// --------------------------------------------------

// Antoine has not implemented the command endpoint yet.
// This function will be completed after the backend update.
void handleBackendCommands() {
  // Future implementation:
  // 1. Retrieve commands through HTTP.
  // 2. Validate the received JSON.
  // 3. Update the correct DeviceState.
  // 4. Call applyDeviceStates().
  // 5. Report the new state to the backend.
}

// --------------------------------------------------
// Arduino setup
// --------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("SmartHouse IoT starting");

  initializeDeviceStates();
  setupHardware();
  applyDeviceStates();

  preferences.begin(
      "smarthouse",
      false
  );

  /*
   * Use preferences.clear() only if Antoine deletes
   * the registered devices from the backend and you
   * intentionally need to register all devices again.
   */
  // preferences.clear();

  delay(1000);
  readSensors();

  if (connectToWiFi()) {
    registerMissingDevices();
    sendAllHeartbeats();

    lastHeartbeatTime = millis();
    lastRegistrationRetryTime = millis();
  }
}

// --------------------------------------------------
// Arduino loop
// --------------------------------------------------

void loop() {
  unsigned long currentTime = millis();

  if (WiFi.status() != WL_CONNECTED) {
    if (
        currentTime - lastWiFiRetryTime >=
        WIFI_RETRY_INTERVAL_MS
    ) {
      lastWiFiRetryTime = currentTime;

      if (connectToWiFi()) {
        registerMissingDevices();
        sendAllHeartbeats();

        lastHeartbeatTime = millis();
        lastRegistrationRetryTime = millis();
      }
    }

    delay(100);
    return;
  }

  if (
      currentTime - lastSensorReadTime >=
      SENSOR_INTERVAL_MS
  ) {
    lastSensorReadTime = currentTime;

    readSensors();
  }

  if (
      hasMissingDeviceSecrets() &&
      currentTime - lastRegistrationRetryTime >=
          REGISTRATION_RETRY_INTERVAL_MS
  ) {
    lastRegistrationRetryTime = currentTime;

    registerMissingDevices();
  }

  if (
      currentTime - lastHeartbeatTime >=
      HEARTBEAT_INTERVAL_MS
  ) {
    lastHeartbeatTime = currentTime;

    sendAllHeartbeats();
  }

  handleBackendCommands();
  applyDeviceStates();

  delay(100);
}
