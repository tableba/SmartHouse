import express from 'express';

import {
  createUser,
  getUsers,
  getUser,
  registerUser,
  loginUser,
  logoutUser
} from '../controllers/usersController.js';

const router = express.Router();

router.get('/users', getUsers);
router.get('/users/:id', getUser);

router.post('/auth/register', registerUser);
router.post('/auth/login', loginUser);
router.post('/auth/logout', logoutUser);

export default router;
