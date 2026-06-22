import express from 'express';
import cors from 'cors';

import config from './config.js';
import usersRoute from './src/routes/usersRoute.js';
import devicesRoute from './src/routes/devicesRoute.js';

const app = express();

app.use(cors());
app.use(express.json());

//routes
app.use('/api', usersRoute);
app.use('/api', devicesRoute);

app.listen(config.port, () =>
  console.log(`Server is live @ ${config.hostUrl}`),
);
