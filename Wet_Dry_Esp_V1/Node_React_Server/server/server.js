const express = require('express');
const bodyParser = require('body-parser');
const path = require('path');
const cors = require('cors');

const app = express();
const PORT = 5000;

// === Middleware ===
app.use(cors());
app.use(express.json());
app.use(bodyParser.json());

// === Internal State ===
let gpioStates = {
  led: "off",
  mix1: "off",
  mix2: "off",
  mix3: "off"
};

let tempHistory = []; // <-- stores temperature readings

// === Routes ===

// GET current GPIO states (used by frontend to poll)
app.get('/led-state', (req, res) => {
  res.json(gpioStates);
});

// POST a GPIO change (e.g., turn LED/motor on or off)
app.post('/set-led', (req, res) => {
  const { name, state } = req.body;
  if (!name || !state || !gpioStates.hasOwnProperty(name)) {
    return res.status(400).json({ error: 'Expected JSON with "name" and "state"' });
  }
  gpioStates[name] = state;
  res.json({ [name]: state });
});

// POST or GET temperature data from the ESP32
app.route('/adc-data')
  .post((req, res) => {
    console.log("Received POST:", req.body);

    if (!req.body || typeof req.body.temperature !== 'number') {
      return res.status(400).json({ error: 'Expected JSON with "temperature"' });
    }

    tempHistory.push(req.body.temperature);
    if (tempHistory.length > 50) tempHistory.shift();

    res.json({ status: "ok", received: req.body.temperature });
  })
  .get((req, res) => {
    res.json({ history: tempHistory });
  });

// === Static frontend ===
app.use(express.static(path.join(__dirname, '../client/build')));
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, '../client/build/index.html'));
});

app.listen(PORT, '0.0.0.0', () => {
  console.log(`✅ Server running at http://0.0.0.0:${PORT}`);
});
