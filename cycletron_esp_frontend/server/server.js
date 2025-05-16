const http = require('http');
const express = require('express');
const path = require('path');
const WebSocket = require('ws');
const sqlite3 = require('sqlite3').verbose();
const fs = require('fs');

const app = express();
const PORT = 5175;

const recoveryFile = 'recovery_state.json';
const gpioFile = 'gpio_state.json';

let recoveryState = {
  machineStep: 'idle',
  lastAction: null,
  progress: 0
};

let gpioState = {};

if (fs.existsSync(recoveryFile)) {
  try {
    recoveryState = JSON.parse(fs.readFileSync(recoveryFile));
    console.log("Loaded recovery state:", recoveryState);
  } catch (e) {
    console.error("Failed to load recovery state file:", e);
  }
}

if (fs.existsSync(gpioFile)) {
  try {
    gpioState = JSON.parse(fs.readFileSync(gpioFile));
    console.log("Loaded GPIO state:", gpioState);
  } catch (e) {
    console.error("Failed to load GPIO state file:", e);
  }
}

function saveRecoveryState() {
  fs.writeFileSync(recoveryFile, JSON.stringify(recoveryState, null, 2));
}

function saveGpioState() {
  fs.writeFileSync(gpioFile, JSON.stringify(gpioState, null, 2));
}

// ----------------- SQLite DB Setup -----------------
const db = new sqlite3.Database('esp_data.db');
db.run(`CREATE TABLE IF NOT EXISTS temperature_log (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
  value REAL
)`);

// ----------------- API Routes -----------------
app.get('/api/history', (req, res) => {
  db.all('SELECT * FROM temperature_log ORDER BY id DESC LIMIT 100', (err, rows) => {
    if (err) return res.status(500).json({ error: err });
    res.json(rows.reverse());
  });
});

// ----------------- Static Frontend -----------------
app.use(express.static(path.join(__dirname, '../dist')));

// ----------------- WebSocket Setup -----------------
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// Maintain references to ESP32 and frontend clients
const clients = new Set();
const espClients = new Set();

wss.on('connection', (ws) => {
  clients.add(ws);
  console.log('New WebSocket connection established');

  ws.on('message', (message) => {
    try {
      const msg = JSON.parse(message);

      // Detect ESP32 by a message property (e.g., type === 'esp_announce')
      if (msg.from === 'esp32') {
        espClients.add(ws);
        console.log('Registered ESP32 WebSocket client');
      }

      // Handle incoming message types
      if (msg.type === 'temperature') {
        db.run('INSERT INTO temperature_log (value) VALUES (?)', [msg.value]);
        console.log(`Logged temperature: ${msg.value}°C`);
        broadcastExcept(ws, JSON.stringify({ type: 'temperatureUpdate', value: msg.value }));
      }

      if (msg.type === 'gpioCommand') {
        gpioState[msg.name] = msg.state;
        saveGpioState();
        console.log(`Updated GPIO: ${msg.name} -> ${msg.state}`);
        broadcastExcept(ws, JSON.stringify({ type: 'gpioStateUpdate', name: msg.name, state: msg.state }));
      }

      if (msg.type === 'getRecoveryState') {
        ws.send(JSON.stringify({
          type: 'recoveryState',
          data: recoveryState
        }));
      }

      if (msg.type === 'updateRecoveryState') {
        recoveryState = { ...recoveryState, ...msg.data };
        saveRecoveryState();
        console.log('Updated recovery state:', recoveryState);
        broadcastExcept(ws, JSON.stringify({ type: 'recoveryState', data: recoveryState }));
      }

    } catch (e) {
      console.error('Bad message:', e);
    }
  });

  ws.on('close', () => {
    clients.delete(ws);
    espClients.delete(ws);
    console.log('WebSocket client disconnected');
  });

  ws.on('error', (err) => {
    console.error('WebSocket error:', err);
  });

  // Initial state on connect
  ws.send(JSON.stringify({ type: 'initialGpioState', data: gpioState }));
});

// Broadcast helper
function broadcastExcept(sender, message) {
  for (const client of clients) {
    if (client !== sender && client.readyState === WebSocket.OPEN) {
      client.send(message);
    }
  }
}

server.listen(PORT, () => {
  console.log(`HTTP & WebSocket Server listening on http://localhost:${PORT}`);
});
