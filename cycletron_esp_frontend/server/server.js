const express = require('express');
const WebSocket = require('ws');
const sqlite3 = require('sqlite3').verbose();
const path = require('path');

const app = express();
const PORT = 5000;

// SQLite DB setup
const db = new sqlite3.Database('esp_data.db');
db.run(`CREATE TABLE IF NOT EXISTS temperature_log (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
  value REAL
)`);

// REST API to get data
app.get('/api/history', (req, res) => {
  db.all('SELECT * FROM temperature_log ORDER BY id DESC LIMIT 100', (err, rows) => {
    if (err) return res.status(500).json({ error: err });
    res.json(rows.reverse()); // so newest is last
  });
});

// Serve frontend
app.use(express.static(path.join(__dirname, '../dist')));

const server = app.listen(PORT, () => {
  console.log(`HTTP Server listening on http://localhost:${PORT}`);
});

// WebSocket server (for ESP32 to connect)
const wss = new WebSocket.Server({ server });

wss.on('connection', (ws) => {
  console.log('ESP32 connected');

  ws.on('message', (message) => {
    try {
      const msg = JSON.parse(message);
      if (msg.type === 'temperature') {
        db.run('INSERT INTO temperature_log (value) VALUES (?)', [msg.value]);
        console.log(`Logged temperature: ${msg.value}°C`);
      }
    } catch (e) {
      console.error('Bad message:', e);
    }
  });

  ws.on('close', () => {
    console.log('ESP32 disconnected');
  });
});
