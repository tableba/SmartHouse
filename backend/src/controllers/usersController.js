import { db } from '../firebase.js';
import User from '../models/userModel.js';
import {
  collection,
  doc,
  addDoc,
  getDoc,
  getDocs,
  updateDoc,
  deleteDoc,
} from 'firebase/firestore';


export const createUser = async (req, res, next) => {
  try {
    const data = req.body;
    await addDoc(collection(db, 'users'), data);
    res.status(200).send('user created successfully');
  } catch (error) {
    res.status(400).send(error.message);
  }
};

export const getUsers = async (req, res, next) => {
  try {
    const users = await getDocs(collection(db, 'users'));
    const userArray = [];

    if (users.empty) {
      res.status(400).send('No users found');
    } else {
      users.forEach((doc) => {
        const user = new User(
          doc.id,
          doc.data().email,
          doc.data().passwordHash,
        );
        userArray.push(user);
      });

      res.status(200).send(userArray);
    }
  } catch (error) {
    res.status(400).send(error.message);
  }
};

export const getUser = async (req, res, next) => {}

export const registerUser = async (req, res, next) => {}

export const loginUser = async (req, res, next) => {}

export const logoutUser = async (req, res, next) => {}
