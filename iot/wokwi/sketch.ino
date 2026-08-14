#include <WiFi.h>
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

// --------------------------------------------------
// Mock backend configuration
// --------------------------------------------------

// Backend is simulated locally inside the ESP32 code.
// No Ngrok or Private Wokwi Gateway is required.
const bool USE_MOCK_BACKEND = true;

// --------------------------------------------------
// Timing
// --------------------------------------------------

const unsigned long HEARTBEAT_INTERVAL_MS = 30000;
const unsigned long STATE_POLL_INTERVAL_MS = 3000;
const unsigned long SENSOR_INTERVAL_MS = 2000;

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
// Sensors and actuators
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

// --------------------------------------------------
// 10 logical SmartHouse devices
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

// Used to change mock states between requests.
unsigned int mockStateStep = 0;

// --------------------------------------------------
// Initial device states
// --------------------------------------------------

void initializeDeviceStates() {
  for (
      size_t index = 0;
      index < DEVICE_COUNT;
      index++
  ) {
    deviceStates[index] =
        DeviceState{};
  }

  // Lights
  deviceStates[0].power = false;
  deviceStates[0].brightness = 0;

  deviceStates[1].power = false;
  deviceStates[1].brightness = 0;

  // Doors
  deviceStates[2].open = false;
  deviceStates[3].open = false;

  // Fan
  deviceStates[4].power = false;
  deviceStates[4].speed = 0;

  // Coffee machine
  deviceStates[5].power = false;
  deviceStates[5].brewing = false;

  // Temperature sensor
  deviceStates[6].temperature = 22.0;

  // Motion sensor
  deviceStates[7].motion = false;

  // Alarm
  deviceStates[8].active = false;

  // Window
  deviceStates[9].open = false;
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
// Apply device states to Wokwi hardware
// --------------------------------------------------

void applyDeviceStates() {

  // Living room light
  digitalWrite(
      LIVING_ROOM_LIGHT_PIN,
      deviceStates[0].power
          ? HIGH
          : LOW
  );

  // Kitchen light
  digitalWrite(
      KITCHEN_LIGHT_PIN,
      deviceStates[1].power
          ? HIGH
          : LOW
  );

  // Front door
  frontDoorServo.write(
      deviceStates[2].open
          ? 90
          : 0
  );

  // Back door
  backDoorServo.write(
      deviceStates[3].open
          ? 90
          : 0
  );

  // Fan
  digitalWrite(
      FAN_PIN,
      deviceStates[4].power
          ? HIGH
          : LOW
  );

  // Coffee machine
  digitalWrite(
      COFFEE_MACHINE_PIN,
      (
        deviceStates[5].power ||
        deviceStates[5].brewing
      )
          ? HIGH
          : LOW
  );

  // Alarm
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

  // Window
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

  Serial.print(
      "Temperature: "
  );

  Serial.print(
      deviceStates[6].temperature
  );

  Serial.print(
      " C, Motion: "
  );

  Serial.println(
      deviceStates[7].motion
          ? "true"
          : "false"
  );
}

// --------------------------------------------------
// Wi-Fi
// --------------------------------------------------

bool connectToWiFi() {
  if (
      WiFi.status() ==
      WL_CONNECTED
  ) {
    return true;
  }

  Serial.print(
      "Connecting to WiFi"
  );

  WiFi.mode(
      WIFI_STA
  );

  WiFi.begin(
      WIFI_SSID,
      WIFI_PASSWORD,
      WIFI_CHANNEL
  );

  unsigned long startTime =
      millis();

  while (
      WiFi.status() !=
          WL_CONNECTED &&
      millis() - startTime <
          20000
  ) {
    delay(250);

    Serial.print(".");
  }

  Serial.println();

  if (
      WiFi.status() !=
      WL_CONNECTED
  ) {
    Serial.println(
        "WiFi connection failed"
    );

    return false;
  }

  Serial.println(
      "WiFi connected"
  );

  Serial.print(
      "IP address: "
  );

  Serial.println(
      WiFi.localIP()
  );

  return true;
}

// --------------------------------------------------
// Find device by ID
// --------------------------------------------------

int findDeviceIndex(
    const char* deviceId
) {
  if (
      deviceId == nullptr
  ) {
    return -1;
  }

  for (
      size_t index = 0;
      index < DEVICE_COUNT;
      index++
  ) {
    if (
        strcmp(
            devices[index].id,
            deviceId
        ) == 0
    ) {
      return
          static_cast<int>(
              index
          );
    }
  }

  return -1;
}

// --------------------------------------------------
// MOCK: Device registration endpoint
// POST /devices/register
// --------------------------------------------------

bool registerDevice(
    size_t deviceIndex
) {
  const DeviceConfig& device =
      devices[deviceIndex];

  String existingSecret =
      preferences.getString(
          device.id,
          ""
      );

  if (
      existingSecret.length() > 0
  ) {
    Serial.print(
        device.id
    );

    Serial.println(
        ": stored mock secret found, registration skipped"
    );

    return true;
  }

  Serial.print(
      "MOCK POST /devices/register -> "
  );

  Serial.println(
      device.id
  );

  // Simulate backend-generated secret.
  String mockSecret =
      "mock_secret_";

  mockSecret +=
      device.id;

  preferences.putString(
      device.id,
      mockSecret
  );

  Serial.print(
      "Registration status for "
  );

  Serial.print(
      device.id
  );

  Serial.println(
      ": 200"
  );

  Serial.print(
      device.id
  );

  Serial.println(
      ": registered and mock secret stored"
  );

  return true;
}

void registerMissingDevices() {
  Serial.println();

  Serial.println(
      "Checking device registrations"
  );

  for (
      size_t index = 0;
      index < DEVICE_COUNT;
      index++
  ) {
    registerDevice(index);

    delay(100);
  }
}

// --------------------------------------------------
// MOCK: Heartbeat endpoint
// POST /devices/heartbeat
// --------------------------------------------------

bool sendHeartbeat(
    size_t deviceIndex
) {
  const DeviceConfig& device =
      devices[deviceIndex];

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
        ": heartbeat failed because no secret is stored"
    );

    return false;
  }

  Serial.print(
      "MOCK POST /devices/heartbeat -> "
  );

  Serial.print(
      device.id
  );

  Serial.println(
      " : 200"
  );

  return true;
}

void sendAllHeartbeats() {
  Serial.println();

  Serial.println(
      "Sending device heartbeats"
  );

  for (
      size_t index = 0;
      index < DEVICE_COUNT;
      index++
  ) {
    sendHeartbeat(index);

    delay(100);
  }

  Serial.println(
      "Heartbeat cycle completed"
  );
}

// --------------------------------------------------
// Apply desired state from simulated backend
// --------------------------------------------------

void applyDesiredState(
    size_t deviceIndex,
    JsonObject state
) {
  switch (
      devices[deviceIndex].kind
  ) {

    case LIGHT:

      if (
          !state["power"].isNull()
      ) {
        deviceStates[
            deviceIndex
        ].power =
            state["power"]
                .as<bool>();
      }

      if (
          !state["brightness"].isNull()
      ) {
        deviceStates[
            deviceIndex
        ].brightness =
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
        deviceStates[
            deviceIndex
        ].open =
            state["open"]
                .as<bool>();
      }

      break;

    case FAN:

      if (
          !state["power"].isNull()
      ) {
        deviceStates[
            deviceIndex
        ].power =
            state["power"]
                .as<bool>();
      }

      if (
          !state["speed"].isNull()
      ) {
        deviceStates[
            deviceIndex
        ].speed =
            state["speed"]
                .as<int>();
      }

      break;

    case COFFEE_MACHINE:

      if (
          !state["power"].isNull()
      ) {
        deviceStates[
            deviceIndex
        ].power =
            state["power"]
                .as<bool>();
      }

      if (
          !state["brewing"].isNull()
      ) {
        deviceStates[
            deviceIndex
        ].brewing =
            state["brewing"]
                .as<bool>();
      }

      break;

    case ALARM:

      if (
          !state["active"].isNull()
      ) {
        deviceStates[
            deviceIndex
        ].active =
            state["active"]
                .as<bool>();
      }

      break;

    case WINDOW:

      if (
          !state["open"].isNull()
      ) {
        deviceStates[
            deviceIndex
        ].open =
            state["open"]
                .as<bool>();
      }

      break;

    case TEMPERATURE_SENSOR:
    case MOTION_SENSOR:

      // Sensor states come from Wokwi.
      break;
  }
}

// --------------------------------------------------
// Create mock GET /devices/states response
// --------------------------------------------------

void createMockBackendResponse(
    JsonDocument& document
) {
  JsonArray array =
      document.to<JsonArray>();

  // Cycle through 4 different demo states.
  int phase =
      mockStateStep % 4;

  // ------------------------------------------------
  // light001
  // ------------------------------------------------

  JsonObject light1 =
      array.add<JsonObject>();

  light1["id"] =
      "light001";

  JsonObject light1State =
      light1["state"]
          .to<JsonObject>();

  light1State["power"] =
      phase == 1 ||
      phase == 2;

  light1State["brightness"] =
      phase == 1
          ? 50
          : phase == 2
              ? 100
              : 0;

  // ------------------------------------------------
  // light002
  // ------------------------------------------------

  JsonObject light2 =
      array.add<JsonObject>();

  light2["id"] =
      "light002";

  JsonObject light2State =
      light2["state"]
          .to<JsonObject>();

  light2State["power"] =
      phase == 2;

  light2State["brightness"] =
      phase == 2
          ? 100
          : 0;

  // ------------------------------------------------
  // door001
  // ------------------------------------------------

  JsonObject door1 =
      array.add<JsonObject>();

  door1["id"] =
      "door001";

  JsonObject door1State =
      door1["state"]
          .to<JsonObject>();

  door1State["open"] =
      phase == 2;

  // ------------------------------------------------
  // door002
  // ------------------------------------------------

  JsonObject door2 =
      array.add<JsonObject>();

  door2["id"] =
      "door002";

  JsonObject door2State =
      door2["state"]
          .to<JsonObject>();

  door2State["open"] =
      phase == 3;

  // ------------------------------------------------
  // fan001
  // ------------------------------------------------

  JsonObject fan =
      array.add<JsonObject>();

  fan["id"] =
      "fan001";

  JsonObject fanState =
      fan["state"]
          .to<JsonObject>();

  fanState["power"] =
      phase == 1 ||
      phase == 2;

  fanState["speed"] =
      phase == 1
          ? 1
          : phase == 2
              ? 3
              : 0;

  // ------------------------------------------------
  // coffee001
  // ------------------------------------------------

  JsonObject coffee =
      array.add<JsonObject>();

  coffee["id"] =
      "coffee001";

  JsonObject coffeeState =
      coffee["state"]
          .to<JsonObject>();

  coffeeState["power"] =
      phase == 2;

  coffeeState["brewing"] =
      phase == 2;

  // ------------------------------------------------
  // temp001
  // ------------------------------------------------

  JsonObject temp =
      array.add<JsonObject>();

  temp["id"] =
      "temp001";

  JsonObject tempState =
      temp["state"]
          .to<JsonObject>();

  tempState["temperature"] =
      deviceStates[6].temperature;

  // ------------------------------------------------
  // motion001
  // ------------------------------------------------

  JsonObject motion =
      array.add<JsonObject>();

  motion["id"] =
      "motion001";

  JsonObject motionState =
      motion["state"]
          .to<JsonObject>();

  motionState["motion"] =
      deviceStates[7].motion;

  // ------------------------------------------------
  // alarm001
  // ------------------------------------------------

  JsonObject alarm =
      array.add<JsonObject>();

  alarm["id"] =
      "alarm001";

  JsonObject alarmState =
      alarm["state"]
          .to<JsonObject>();

  alarmState["active"] =
      phase == 3;

  // ------------------------------------------------
  // window001
  // ------------------------------------------------

  JsonObject window =
      array.add<JsonObject>();

  window["id"] =
      "window001";

  JsonObject windowState =
      window["state"]
          .to<JsonObject>();

  windowState["open"] =
      phase == 1 ||
      phase == 2;
}

// --------------------------------------------------
// MOCK: GET /devices/states
// --------------------------------------------------

bool fetchDesiredStates() {
  Serial.println();

  Serial.println(
      "MOCK GET /devices/states"
  );

  Serial.println(
      "Device states status: 200"
  );

  JsonDocument responseDocument;

  createMockBackendResponse(
      responseDocument
  );

  JsonArray serverDevices =
      responseDocument
          .as<JsonArray>();

  Serial.print(
      "Received desired states for "
  );

  Serial.print(
      serverDevices.size()
  );

  Serial.println(
      " devices"
  );

  for (
      JsonVariant item :
      serverDevices
  ) {
    const char* deviceId =
        item["id"] | "";

    int deviceIndex =
        findDeviceIndex(
            deviceId
        );

    if (
        deviceIndex < 0
    ) {
      continue;
    }

    JsonObject state =
        item["state"]
            .as<JsonObject>();

    applyDesiredState(
        static_cast<size_t>(
            deviceIndex
        ),
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

  mockStateStep++;

  return true;
}

// --------------------------------------------------
// Print current states
// --------------------------------------------------

void printCurrentStates() {
  Serial.println(
      "Current simulated states:"
  );

  Serial.print(
      "light001 power: "
  );

  Serial.println(
      deviceStates[0].power
          ? "ON"
          : "OFF"
  );

  Serial.print(
      "light002 power: "
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
      "Backend mode: MOCK"
  );

  initializeDeviceStates();

  setupHardware();

  applyDeviceStates();

  preferences.begin(
      "smarthouse",
      false
  );

  /*
   * Uncomment once if you want to delete
   * all previously stored mock secrets.
   */
  // preferences.clear();

  delay(1000);

  readSensors();

  // Wi-Fi is still demonstrated,
  // but mock endpoints do not depend on
  // an external backend server.
  connectToWiFi();

  registerMissingDevices();

  sendAllHeartbeats();

  fetchDesiredStates();

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

  // Read physical/simulated sensors every 2 seconds.
  if (
      currentTime -
      lastSensorReadTime >=
      SENSOR_INTERVAL_MS
  ) {
    lastSensorReadTime =
        currentTime;

    readSensors();
  }

  // Mock GET /devices/states every 3 seconds.
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

  // Mock heartbeat every 30 seconds.
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