import express from 'express';

import {
  getDevices,
  getDevice,
  registerDevice,
  heartBeat,
  updateDevice,
  deleteDevice
} from '../controllers/devicesController.js';

import { authenticateUser } from '../middleware/auth.js'

const router = express.Router();

// for iot only, best compromise for now
router.get('/devices/states', getDevices);

// proteced
router.get('/devices',
  authenticateUser,
  getDevices);
router.get('/devices/:id',
  authenticateUser,
  getDevice)
router.delete('/devices/:id',
  authenticateUser,
  deleteDevice);
router.put('/devices/:id',
  authenticateUser,
  updateDevice);

// unprotected
router.post('/devices/register', registerDevice);
router.post('/devices/heartbeat', heartBeat);


export default router;
