const express = require('express');
const bodyParser = require('body-parser');
const path = require('path');
const cors = require('cors');

const app = express();
const PORT = 5000;

//  Middleware setup
app.use(cors());
app.use(express.json());
app.use(bodyParser.json());

//  Simulated backend state
let gpioStates = {
  led: "off",
  mix1: "off",
  mix2: "off",
  mix3: "off"
};
let adcHistory = [];


// GET current GPIO states (e.g., for polling)
app.get('/led-state', (req, res) => {
  res.json(gpioStates);
});

// POST to set any single GPIO state (led, mix1, mix2, mix3)
app.post('/set-led', (req, res) => {
  const { name, state } = req.body;
  if (!name || !state || !gpioStates.hasOwnProperty(name)) {
    return res.status(400).json({ error: 'Expected JSON with "name" and "state"' });
  }
  gpioStates[name] = state;
  res.json({ [name]: state });
});

app.route('/adc-data')
  .post((req, res) => {
    if (!req.body || typeof req.body.adc !== 'number') {
      return res.status(400).json({ error: 'Expected JSON with "adc"' });
    }
    adcHistory.push(req.body.adc);
    if (adcHistory.length > 50) adcHistory.shift();
    res.json({ status: "ok", received: req.body.adc });
  })
  .get((req, res) => {
    res.json({ history: adcHistory });
  });

//  Static React frontend (moved to bottom)
app.use(express.static(path.join(__dirname, '../client/build')));
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, '../client/build/index.html'));
});

app.listen(PORT, '0.0.0.0', () => {
  console.log(`✅ Server running at http://0.0.0.0:${PORT}`);
});
