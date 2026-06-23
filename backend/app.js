import express from 'express';
import cors from 'cors';
import {
  doc,
  collection,
  getDocs,
  updateDoc
} from 'firebase/firestore';

import config from './config.js';
import usersRoute from './src/routes/usersRoute.js';
import devicesRoute from './src/routes/devicesRoute.js';
import { db } from './src/firebase.js';


const app = express();
const TIMEOUT = 40000; // 30 seconds

app.use(cors());
app.use(express.json());

//routes
app.use('/api', usersRoute);
app.use('/api', devicesRoute);


async function checkOfflineDevices() {
  const snapshot = await getDocs(collection(db, "devices"));

  const now = Date.now();

  snapshot.forEach(async (d) => {
    const data = d.data();

    if (!data.lastSeen) return;

    const lastSeen = new Date(data.lastSeen).getTime();

    if (now - lastSeen > TIMEOUT && data.status !== "offline") {
      await updateDoc(doc(db, "devices", d.id), {
        status: "offline"
      });
    }
  });
}

// run every 10 seconds
setInterval(checkOfflineDevices, 10000);

app.listen(config.port, () =>
  console.log(`Server is live @ ${config.hostUrl}`),
);
