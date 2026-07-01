import { db } from '../firebase.js';
import Device from '../models/deviceModel.js';
import {
  collection,
  doc,
  addDoc,
  setDoc,
  getDoc,
  getDocs,
  updateDoc,
  deleteDoc,
} from 'firebase/firestore';


export const createDevice = async (req, res, next) => {
  try {
    const device = Device.create(req.body);
    await setDoc(doc(db, "devices", device.id), device.toJSON());
    res.status(200).json({
      message: "device added to database sucessfully",
      secret: device.secret
    });
  } catch (error) {
    res.status(400).send(error.message);
  }
};

export const getDevices = async (req, res, next) => {
  try {
    const snapshot = await getDocs(collection(db, "devices"));

    if (snapshot.empty) {
      return res.status(404).json({ message: "No devices found" });
    }

    const deviceArray = [];

    snapshot.forEach((doc) => {
      const data = doc.data();

      const device = new Device({
        id: doc.id,
        name: data.name,
        type: data.type,
        state: data.state || {},
        status: data.status || "offline",
        registeredAt: data.registeredAt,
        lastModified: data.lastModified,
        lastSeen: data.lastSeen
      });

      deviceArray.push(device);
    });

    return res.status(200).json(deviceArray);
  } catch (error) {
    return res.status(500).json({
      message: error.message
    });
  }
}

export const getDevice = async (req, res, next) => {
  try {
    const { id } = req.params
    const deviceRef = doc(db, "devices", id)
    const snapshot = await getDoc(deviceRef);

    if (!snapshot.exists()) {
      return res.status(404).json({ message: "No device found" });
    }

    const data = snapshot.data()

    const device = new Device({
      id: id,
      name: data.name,
      type: data.type,
      state: data.state || {},
      status: data.status || "offline",
      registeredAt: data.registeredAt,
      lastModified: data.lastModified,
      lastSeen: data.lastSeen
    });

    return res.status(200).json(device.toJSON());

  } catch (error) {
    return res.status(500).json({
      message: error.message
    });
  }
}

export const authentificateDevice = async (req, res, next) => {
  // authentificates devices and makes it "online"
  try {
    const { id, secret } = req.body;
    const deviceRef = doc(db, "devices", id);
    const snapshot = await getDoc(deviceRef);

    if (!snapshot.exists()) {
      return res.status(404).json({
        message: "Device not found"
      });
    }

    const device = snapshot.data();

    if (device.secret !== secret) {
      return res.status(401).json({
        message: "Invalid credentials"
      });
    }

    await updateDoc(deviceRef, {
      status: "online",
      lastSeen: new Date().toISOString()
    });

    return res.status(200).json({
      message: "Authenticated",
      deviceId: device.id
    });

  } catch (error) {
    return res.status(500).json({
      message: error.message
    });
  }
};

export const heartBeat = async (req, res, next) => {
  try {
    const { id, secret } = req.body;
    const deviceRef = doc(db, "devices", id);
    const snapshot = await getDoc(deviceRef);

    if (!snapshot.exists()) {
      return res.status(404).json({
        message: "Device not found"
      });
    }

    const device = snapshot.data();

    if (device.secret !== secret) {
      return res.status(401).json({
        message: "Invalid credentials"
      });
    }

    await updateDoc(deviceRef, {
      status: "online",
      lastSeen: new Date().toISOString()
    })
    
    return res.status(200).json({ message: "heartbeat ok" })

  } catch (error) {
    return res.status(500).json({
      message: error.message
    });
  }
}

// combine auth and adding device to db
export const registerDevice = async (req, res) => {
  try {
    const { id, secret } = req.body;

    const deviceRef = doc(db, "devices", id);
    const snapshot = await getDoc(deviceRef);

    // device does not exist in db (based on id)
    if (!snapshot.exists()) {

      const device = Device.create(req.body);

      await setDoc(
        deviceRef,
        device.toJSON()
      );

      await updateDoc(deviceRef, {
        status: "online",
        lastSeen: new Date().toISOString()
      });

      return res.status(201).json({
        message: "Device registered",
        deviceId: device.id,
        secret: device.secret
      });
    }

      return res.status(401).json({
        message: "Device with that id already exists."
      });

  } catch (error) {
    return res.status(500).json({
      message: error.message
    });
  }
};


export const deleteDevice = async (req, res, next) => {
  try {

    const { id } = req.params
    const deviceRef = doc(db, "devices", id)
    const snapshot = await getDoc(deviceRef);

    if (!snapshot.exists()) {
      return res.status(404).json({ message: "Device not found" });
    }

    await deleteDoc(doc(db, "devices", id));
    res.status(200).json({
      message: "device removed from the database sucessfully",
    });
  } catch (error) {
    res.status(500).send(error.message);
  }
};

export const updateDevice = async (req, res, next) => {
  // can only updatee name and type!
  try {
    const { id } = req.params
    const deviceRef = doc(db, "devices", id)
    const snapshot = await getDoc(deviceRef);

    if (!snapshot.exists()) {
      return res.status(404).json({ message: "Device not found" });
    }

    const updates = req.body

    if (updates.name && typeof updates.name !== "string") {
      return res.status(400).json({
        message: "Invalid name"
      });
    }

    if (updates.type && !Device.isValidType(updates.type)) {
      return res.status(400).json({
        message: "Invalid type"
      });
    }

    await updateDoc(deviceRef, {
      ... updates,
      lastModified: new Date().toISOString()
    });

    res.status(200).json({
      message: "device updated sucessfully",
    });
  } catch (error) {
    res.status(500).send(error.message);
  }
};
