import express from 'express';

import {
  createUser,
  getUsers,
} from '../controllers/usersController.js';

const router = express.Router();

router.get('/', getUsers);
router.post('/new', createUser);

export default router;
