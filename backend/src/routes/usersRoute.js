import express from 'express';

import {
  createUser,
  getUsers,
  getUser,
  register,
  login,
  logout
} from '../controllers/usersController.js';

import { authenticateUser } from '../middleware/auth.js'

const router = express.Router();

router.get('/users',
  authenticateUser,
  getUsers);
router.get('/users/:id',
  authenticateUser,
  getUser);

router.post('/users/register', register);
router.post('/users/login', login);
router.post('/users/logout',
  authenticateUser,
  logout);

export default router;
