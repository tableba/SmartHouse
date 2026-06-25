import {
  collection,
  doc,
  setDoc,
  addDoc,
  getDoc,
  getDocs,
  updateDoc,
  deleteDoc,
} from 'firebase/firestore';
import jwt from 'jsonwebtoken';
import bcrypt from 'bcrypt';

import { db } from '../firebase.js';
import User from '../models/userModel.js';

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
          doc.data().createdAt,

        );
        userArray.push(user);
      });

      res.status(200).send(userArray);
    }
  } catch (error) {
    res.status(400).send(error.message);
  }
};

export const getUser = async (req, res, next) => {
  try {
    const id = req.params.id
    const snapshot = await getDoc(
      doc(db, "users", id)
    );

    if (!snapshot.exists()) {
      res.status(400).send('No user found');
    } else {
      const user = new User(
        doc.id,
        doc.data().email,
        doc.data().passwordHash,
        doc.data().createdAt,
      );
    };
    res.status(200).send(user);
  } catch (error) {
    res.status(400).send(error.message);
  }
};

export const register = async (req, res) => {
  try {
    const { email, password } = req.body;

    // hash password
    const passwordHash = await bcrypt.hash(password, 10);

    await setDoc(doc(db, "users", email), {
      email,
      passwordHash,
      createdAt: new Date().toISOString()
    });

    return res.status(201).json({
      message: "User created"
    });

  } catch (error) {
    res.status(400).send(error.message);
  }
};

export const login = async (req, res, next) => {
  try {
    const { email, password } = req.body;

    const snapshot = await getDoc(
      doc(db, "users", email)
    );

    if (!snapshot.exists()) {
      return res.status(400).json({
        message: "User not found"
      })
    }

    const user = snapshot.data()

    const validPassword = await bcrypt.compare(
      password,
      user.passwordHash
    );

    if (!validPassword) {
      return res.status(401).json({
        message: "invalid credentials"
      })
    }
    const token = jwt.sign(
      {
        email: user.email
      },
      process.env.JWT_SECRET,
      {
        expiresIn: "24h"
      }
    );

    console.log(process.env.JWT_SECRET)

    return res.status(200).json({
      token
    })

  } catch (error) {
    res.status(400).send(error.message);
  }
}

export const logout = async (req, res, next) => {}
