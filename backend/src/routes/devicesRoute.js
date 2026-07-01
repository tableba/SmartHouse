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

router.post('/devices/register', registerDevice);
router.post('/devices/heartbeat', heartBeat);

export default router;
