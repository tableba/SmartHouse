import express from 'express';

import {
  getUsers,
  getUser,
  deleteUser,
  register,
  login,
} from '../controllers/usersController.js';

import { authenticateUser } from '../middleware/auth.js'

const router = express.Router();

router.get('/users',
  authenticateUser,
  getUsers);
router.get('/users/:id',
  authenticateUser,
  getUser);
router.delete('/users/:id',
  authenticateUser,
  deleteUser);

router.post('/users/register', register);
router.post('/users/login', login);

export default router;
