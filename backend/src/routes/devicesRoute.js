import express from 'express';

import {
  getDevices,
  createDevice,
  authentificateDevice,
  heartBeat,
} from '../controllers/devicesController.js';

const router = express.Router();

router.get('/devices', getDevices);
router.post('/devices', createDevice);

router.post('/devices/auth', authentificateDevice);
router.post('/devices/heartbeat', heartBeat)

export default router;
