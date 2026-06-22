import express from 'express';

import {
  getDevices,
  createDevice,
} from '../controllers/devicesController.js';

const router = express.Router();

router.get('/devices', getDevices);
router.post('/devices', createDevice);

export default router;
