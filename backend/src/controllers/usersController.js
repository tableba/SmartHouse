import {
  collection,
  query,
  where,
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
    const { id } = req.params
    const snapshot = await getDoc(collection(db, "users", id));

    if (!snapshot.exists()) {
      return res.status(404).json({ message: "No user found" });
    }

    const data = snapshot.data()

    const user = new User({
      id: id,
      email: data.email,
      passwordHash: data.passwordHash,
      createdAt: data.createdAt,
    });

    return res.status(200).json(user.toJSON());

  } catch (error) {
    return res.status(500).send(error.message)
  }
}

export const deleteUser = async (req, res, next) => {
  try {
    const { id } = req.params

    const userRef = doc(db, 'users', id)

    await deleteDoc(userRef)

    return res.status(200).json({ message: "successfully deleted user."})

  } catch (error) {
    return res.status(500).send(error.message)
  }
}

export const register = async (req, res) => {
  try {
    const { email, password } = req.body;

    // hash password
    const passwordHash = await bcrypt.hash(password, 10);

    await addDoc(collection(db, "users"), {
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

    const q = query(
      collection(db, "users"),
      where("email", "==", email)
    );

    const snapshot = await getDocs(q);

    if (snapshot.empty) {
      return res.status(400).json({
        message: "User not found"
      })
    }

    const userDoc = snapshot.docs[0];
    const user = userDoc.data();

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
