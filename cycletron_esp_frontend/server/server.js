const http = require('http');
const express = require('express');
const path = require('path');
const WebSocket = require('ws');
const sqlite3 = require('sqlite3').verbose();
const fs = require('fs');
const { exec } = require('child_process'); // For opening file location in Explorer

const app = express();
const PORT = 5175;

const recoveryFile = 'recovery_state.json';
const espRecoveryFile = 'ESP_Recovery.json'; // File for ESP32 recovery state

let recoveryState = {};
let espRecoveryState = {};

// Load recovery state if the file exists
if (fs.existsSync(recoveryFile)) {
  try {
    recoveryState = JSON.parse(fs.readFileSync(recoveryFile));
    console.log("Loaded recovery state:", recoveryState);
  } catch (e) {
    console.error("Failed to load recovery state file:", e);
    recoveryState = {};
  }
}

// Load ESP recovery state if the file exists
if (fs.existsSync(espRecoveryFile)) {
  try {
    espRecoveryState = JSON.parse(fs.readFileSync(espRecoveryFile));
    console.log("Loaded ESP recovery state:", espRecoveryState);
  } catch (e) {
    console.error("Failed to load ESP recovery state file:", e);
    espRecoveryState = {};
  }
}

function saveRecoveryState() {
  fs.writeFileSync(recoveryFile, JSON.stringify(recoveryState, null, 2));
}

// Function to save ESP recovery state
function saveEspRecoveryState() {
  fs.writeFileSync(espRecoveryFile, JSON.stringify(espRecoveryState, null, 2));
}

// Reset ESP recovery state
function resetEspRecoveryState() {
  if (fs.existsSync(espRecoveryFile)) {
    fs.unlinkSync(espRecoveryFile);
    console.log('ESP_Recovery.json deleted.');
  }
  espRecoveryState = {};
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

// Add this route to reset the recovery state
app.post('/api/resetRecoveryState', (req, res) => {
  if (fs.existsSync(recoveryFile)) {
    fs.unlinkSync(recoveryFile);
    console.log('recovery_state.json deleted by request.');
  }
  recoveryState = {}; // Reset in-memory state
  res.json({ success: true });
});

// Add this route to reset the ESP recovery state
app.post('/api/resetEspRecoveryState', (req, res) => {
  if (fs.existsSync(espRecoveryFile)) {
    fs.unlinkSync(espRecoveryFile);
    console.log('ESP_Recovery.json deleted by request.');
  }
  espRecoveryState = {}; // Reset in-memory state
  res.json({ success: true });
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
  let isEspClient = false; // Track if this client is an ESP32

  if (!clients.has(ws)) {
    clients.add(ws);
    console.log('New WebSocket connection established');
  }

  ws.on('message', (message) => {
    try {
      const msg = JSON.parse(message);

      // Detect ESP32 by a message property (e.g., type === 'esp_announce')
      if (msg.from === 'esp32') {
        if (!espClients.has(ws)) {
          espClients.add(ws);
          isEspClient = true;
          console.log('Registered ESP32 WebSocket client');
          broadcastExcept(ws, JSON.stringify({ type: 'status', status: 'connected' }));

          // Send ESP recovery state to ESP32 on connection
          if (Object.keys(espRecoveryState).length > 0) {
            ws.send(JSON.stringify({ type: 'espRecoveryState', data: espRecoveryState }));
            console.log('Sent ESP recovery state to ESP32:', espRecoveryState);
          } else {
            console.log('ESP recovery state is empty. ESP32 will start fresh.');
          }
        }
        return; // ESP32 never gets recoveryState
      }

      // Handle heartbeat packets from ESP32
      if (msg.type === 'heartbeat') {
        // Forward status update to all frontend clients
        for (const client of clients) {
          if (client.readyState === WebSocket.OPEN && !espClients.has(client)) {
            client.send(JSON.stringify({ type: 'status', status: 'connected' }));
          }
        }
        // Optionally, you can log the heartbeat
        // console.log('Received heartbeat from ESP32');
        return;
      }

      // If a frontend connects, send recoveryState on request or after identification
      if (msg.type === 'getRecoveryState') {
        ws.send(JSON.stringify({
          type: 'recoveryState',
          data: recoveryState
        }));
      }

      // Handle incoming message types
      if (msg.type === 'temperature') {
        db.run('INSERT INTO temperature_log (value) VALUES (?)', [msg.value]);
        console.log(`Logged temperature: ${msg.value}°C`);
        broadcastExcept(ws, JSON.stringify({ type: 'temperatureUpdate', value: msg.value }));
      }

      if (msg.type === 'button' && msg.name === 'startCycle') {
        recoveryState.machineStep = 'started';
        saveRecoveryState();

        // Broadcast recovery state to frontend clients only
        for (const client of clients) {
          if (client !== ws && client.readyState === WebSocket.OPEN && !espClients.has(client)) {
            client.send(JSON.stringify({ type: 'recoveryState', data: recoveryState }));
          }
        }
        console.log('Cycle started, recovery state updated:', recoveryState);
      }

      // Handles button commands
      if (msg.type === 'button') {
        console.log(`Button command received: ${msg.name} -> ${msg.state}`);
        // Forward the button command to all ESP32 clients
        for (const esp of espClients) {
          if (esp.readyState === WebSocket.OPEN) {
            esp.send(JSON.stringify(msg));
          }
        }
      }

      if (msg.type === 'getRecoveryState') {
        // Only send recovery state to frontend clients
        if (!isEspClient) {
          ws.send(JSON.stringify({
            type: 'recoveryState',
            data: recoveryState
          }));
        }
      }

      if (msg.type === 'updateRecoveryState') {
        recoveryState = { ...recoveryState, ...msg.data };
        saveRecoveryState(); // Save the updated recovery state to the file
        console.log('Updated recovery state:', recoveryState);

        // Only broadcast to frontend clients
        for (const client of clients) {
          if (
            client !== ws &&
            client.readyState === WebSocket.OPEN &&
            !espClients.has(client) // Only send to non-ESP32 clients
          ) {
            client.send(JSON.stringify({ type: 'recoveryState', data: recoveryState }));
          }
        }
      }

      if (msg.type === 'parameters') {
        console.log('Received parameters:', msg.data);
        // Normalize parameters to ensure numbers are sent to ESP and stored
        const normalizedParams = normalizeParameters(msg.data);
        // Forward parameters to all ESP32 clients
        for (const esp of espClients) {
          if (esp.readyState === WebSocket.OPEN) {
            esp.send(JSON.stringify({ ...msg, data: normalizedParams }));
          }
        }
        // Update ESP recovery state
        if (normalizedParams && typeof normalizedParams === 'object') {
          espRecoveryState.parameters = { ...espRecoveryState.parameters, ...normalizedParams };
          espRecoveryState.lastUpdated = new Date().toISOString();
          saveEspRecoveryState();
          console.log('ESP_Recovery.json updated with parameters:', espRecoveryState.parameters);
        }
      }

      // --- ADD THIS BLOCK: handle progress and currentState updates ---
      if (msg.type === 'progress' && msg.data && typeof msg.data === 'object') {
        espRecoveryState.progress = { ...espRecoveryState.progress, ...msg.data };
        espRecoveryState.lastUpdated = new Date().toISOString();
        saveEspRecoveryState();
        console.log('ESP_Recovery.json updated with progress:', espRecoveryState.progress);
      }
      if (msg.type === 'currentState' && msg.value) {
        espRecoveryState.currentState = msg.value;
        espRecoveryState.lastUpdated = new Date().toISOString();
        saveEspRecoveryState();
        console.log('ESP_Recovery.json updated with currentState:', espRecoveryState.currentState);
      }
      // --- END BLOCK ---

      if (msg.type === 'button' && msg.name === 'logCycle') {
        // Format: Log_Cycle_YYYY-MM-DD_HH-MM.json (safe for Windows/Mac)
        const now = new Date();
        const pad = n => n.toString().padStart(2, '0');
        const dateStr = `${now.getFullYear()}-${pad(now.getMonth() + 1)}-${pad(now.getDate())}`;
        const timeStr = `${pad(now.getHours())}-${pad(now.getMinutes())}`;
        const logFile = path.join(__dirname, '..', `Log_Cycle_${dateStr}_${timeStr}.json`);

        // Compose log entry with ordered parameters
        const p = msg.parameters || {};
        const orderedParameters = {
          volumeAddedPerCycle: p.volumeAddedPerCycle,
          durationOfRehydration: p.durationOfRehydration,
          syringeDiameter: p.syringeDiameter,
          desiredHeatingTemperature: p.desiredHeatingTemperature,
          durationOfHeating: p.durationOfHeating,
          durationOfMixing: p.durationOfMixing,
          numberOfCycles: p.numberOfCycles,
          sampleZonesToMix: p.sampleZonesToMix,
        };

        const entry = {
          timestamp: msg.timestamp || now.toISOString(),
          parameters: orderedParameters,
          espOutputs: msg.espOutputs || null,
        };

        // Write the entry to a new file
        fs.writeFileSync(logFile, JSON.stringify(entry, null, 2));
        console.log('Logged cycle to:', logFile);

        // Optionally open the file location
        if (process.platform === 'win32') {
          exec(`explorer.exe /select,"${logFile.replace(/\//g, '\\')}"`);
        } else if (process.platform === 'darwin') {
          exec(`open -R "${logFile}"`);
        } else {
          console.log('Automatic file opening not supported on this OS.');
        }
      }

      if (msg.type === 'resetRecoveryState') {
        if (fs.existsSync(recoveryFile)) {
          fs.unlinkSync(recoveryFile);
          console.log('recovery_state.json deleted by frontend request.');
        }
        recoveryState = {};
        // Also reset ESP recovery state
        resetEspRecoveryState();
        // Notify all clients of the reset state
        wss.clients.forEach(client => {
          if (client.readyState === WebSocket.OPEN) {
            client.send(JSON.stringify({ type: 'recoveryState', data: recoveryState }));
          }
        });
        return;
      }

      // Reset ESP recovery state when the frontend presses "End Cycle"
      if (
        msg.type === 'button' &&
        msg.name === 'endCycle' &&
        msg.state === 'on'
      ) {
        resetEspRecoveryState();
      }

      if (msg.type === 'button' && msg.name === 'vialSetup') {
        console.log(`Vial setup status received: ${msg.status}`);
        // Forward the vial setup status to all ESP32 clients
        for (const esp of espClients) {
          if (esp.readyState === WebSocket.OPEN) {
            esp.send(JSON.stringify({ type: 'vialSetup', status: msg.status }));
          }
        }
      }

      // Only store recovery packets from ESP32
      if (msg.type === 'espRecovery' || msg.type === 'espRecoveryState') {
        // If the ESP sends the whole state as msg.data, just store it
        if (msg.data && typeof msg.data === 'object') {
          espRecoveryState = { ...espRecoveryState, ...msg.data };
        }
        // If using the sectioned update format
        if (msg.section === 'parameters' && typeof msg.data === 'object') {
          espRecoveryState.parameters = { ...espRecoveryState.parameters, ...msg.data };
        } else if (msg.section === 'progress' && typeof msg.data === 'object') {
          espRecoveryState.progress = { ...espRecoveryState.progress, ...msg.data };
        } else if (msg.section === 'currentState') {
          espRecoveryState.currentState = msg.data;
        }
        // Always update lastUpdated
        espRecoveryState.lastUpdated = new Date().toISOString();
        saveEspRecoveryState();
        // Optionally log:
        // console.log('ESP Recovery updated:', espRecoveryState);
        return;
      }

      // (rest of the message handler remains unchanged)
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
});

// Broadcast helper
function broadcastExcept(sender, message) {
  for (const client of clients) {
    if (client !== sender && client.readyState === WebSocket.OPEN) {
      client.send(message);
    }
  }
}

// Utility: convert parameter fields to numbers where appropriate
function normalizeParameters(params) {
  if (!params) return {};
  const numericKeys = [
    "volumeAddedPerCycle",
    "syringeDiameter",
    "desiredHeatingTemperature",
    "durationOfHeating",
    "durationOfMixing",
    "numberOfCycles"
  ];
  const out = { ...params };
  for (const key of numericKeys) {
    if (out[key] !== undefined) out[key] = Number(out[key]);
  }
  // sampleZonesToMix should be an array of numbers
  if (Array.isArray(out.sampleZonesToMix)) {
    out.sampleZonesToMix = out.sampleZonesToMix.map(Number);
  }
  return out;
}

server.listen(PORT, () => {
  console.log(`HTTP & WebSocket Server listening on http://localhost:${PORT}`);
});