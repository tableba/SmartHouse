import { randomBytes } from 'crypto'

class Device {
  constructor({
    id,
    name,
    type,
    state = {},
    status = "online",
    registeredAt = null,
    lastModified = null,
    lastSeen = null,
    secret
  }) {
    this.id = id;
    this.name = name;
    this.type = type;

    // dynamic device state (power, brightness, locked, speed, etc.)
    this.state = state;

    // online/offline/unknown
    this.status = status;

    // timestamps
    this.registeredAt = registeredAt;
    this.lastModified = lastModified;
    this.lastSeen = lastSeen;

    // auth
    this.secret = secret;
  }

static create(data) {

  // input validation
  if (!data.id || typeof data.id !== "string") {
    throw new Error("Device id is required");
  }

  if (!data.name || typeof data.name !== "string") {
    throw new Error("Device name is required");
  }

  if (!data.type || typeof data.type !== "string") {
    throw new Error("Device type is required");
  }

  if (!this.isValidType(data.type)) {
      throw new Error(`Invalid device type: ${data.type}`);
    }

  const now = new Date().toISOString();
  const secret = randomBytes(32).toString("hex");

  return new Device({
    ...data,
    state: data.state || {},
    status: "offline",
    registeredAt: now,
    lastModified: now,
    lastSeen: now,
    secret
  });
}

  static isValidType(type) {
  // subject to change maybe
    const validTypes = [
      "light",
      "door",
      "fan",
      "coffee_machine",
      "temperature_sensor",
      "motion_sensor",
      "alarm",
      "window"
    ];

    return validTypes.includes(type);
  }

  markOnline() {
    this.status = "online";
    this.lastSeen = new Date().toISOString();
  }

  markOffline() {
    this.status = "offline";
  }

  toJSON() {
    return {
      id: this.id,
      name: this.name,
      type: this.type,
      state: this.state,
      status: this.status,
      registeredAt: this.registeredAt,
      lastModified: this.lastModified,
      lastSeen: this.lastSeen,
      secret: this.secret
    };
  }
}

export default Device;
