import express from 'express';

import usersRoute from './usersRoute.js'

import {
  createUser,
  getUsers,
} from '../controllers/usersController.js.js';

const router = express.Router();

router.get('/', getUsers);
router.post('/new', createUser);

export default router;
