const express = require('express');
const bodyParser = require('body-parser');
const path = require('path');

const app = express();
const PORT = 5000;

let ledState = "off";
let adcHistory = [];

app.use(bodyParser.json());
app.use(express.static(path.join(__dirname, '../client/build')));

app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, '../client/build/index.html'));
});

app.get('/led-state', (req, res) => {
  res.json({ led: ledState });
});

app.post('/set-led', (req, res) => {
  if (!req.body || typeof req.body.state !== 'string') {
    return res.status(400).json({ error: 'Expected JSON with "state"' });
  }
  ledState = req.body.state;
  res.json({ led: ledState });
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

app.listen(PORT, () => {
  console.log(`Server running at http://localhost:${PORT}`);
});
