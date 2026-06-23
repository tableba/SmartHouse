import { randomBytes } from 'crypto'

class Device {
  constructor({
    id,
    name,
    type,
    state = {},
    status = "offline",
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
    const now = new Date().toISOString();
    const secret = randomBytes(32).toString("hex");

    return new Device({
      ...data,
      status: "offline",
      registeredAt: now,
      lastModified: now,
      lastSeen: now,
      secret: secret
    });
  }

  updateState(newState) {
    this.state = {
      ...this.state,
      ...newState
    };

    this.lastModified = new Date().toISOString();
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
