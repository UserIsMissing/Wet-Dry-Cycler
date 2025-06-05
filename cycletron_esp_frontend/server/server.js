const http = require('http');
const express = require('express');
const path = require('path');
const WebSocket = require('ws');
const sqlite3 = require('sqlite3').verbose();
const fs = require('fs');
const { exec } = require('child_process'); // For opening file location in Explorer

const app = express();
const PORT = 5175;

// Change recovery file paths to be in the /server/ folder and update names
const recoveryFile = path.join(__dirname, 'Frontend_Recovery.json');
const espRecoveryFile = path.join(__dirname, 'ESP_Recovery.json'); // File for ESP32 recovery state

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
  // Delete frontend recovery state
  if (fs.existsSync(recoveryFile)) {
    fs.unlinkSync(recoveryFile);
    console.log('Frontend_Recovery.json deleted by request.');
  }
  
  // Delete ESP32 recovery state
  if (fs.existsSync(espRecoveryFile)) {
    fs.unlinkSync(espRecoveryFile);
    console.log('ESP_Recovery.json deleted by request.');
  }
  
  recoveryState = {}; // Reset frontend in-memory state
  espRecoveryState = {}; // Reset ESP32 in-memory state
  
  res.json({ success: true });
});

// Add route to get recovery state
app.get('/api/recoveryState', (req, res) => {
  res.json({ recoveryState });
});

// ----------------- Static Frontend -----------------
app.use(express.static(path.join(__dirname, '../dist')));

// ----------------- WebSocket Setup -----------------
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// Maintain references to ESP32 and frontend clients
const clients = new Set();
const espClients = new Set();

wss.on('connection', (ws, req) => {
  let isEspClient = false; // Track if this client is an ESP32
  let espRecoverySent = false; // Ensure we only send recovery once per connection

  const clientIP = req.socket.remoteAddress || 'unknown';
  const clientPort = req.socket.remotePort || 'unknown';
  console.log(`New WebSocket connection from ${clientIP}:${clientPort}`);

  if (!clients.has(ws)) {
    clients.add(ws);
    console.log(`Total WebSocket connections: ${clients.size}`);
  }

  ws.on('message', (message) => {
    console.log('[WS DEBUG] Received message:', message);
    try {
      const msg = JSON.parse(message);

      // Debug: log all message types
      if (msg.type) {
        console.log(`[WS DEBUG] Message type: ${msg.type}`);
      }

      // Detect ESP32 by a message property (e.g., type === 'esp_announce')
      if (msg.from === 'esp32') {
        if (!isEspClient) {
          console.log(`[WS DEBUG] NEW ESP32 CLIENT DETECTED! Adding to espClients set.`);
          isEspClient = true;
          espClients.add(ws);
        }
        // On ESP32 connect, send recovery state if available and not already sent
        if (!espRecoverySent && fs.existsSync(espRecoveryFile)) {
          try {
            const fileContent = fs.readFileSync(espRecoveryFile, 'utf8');
            if (fileContent && fileContent.trim() !== '{}' && fileContent.trim() !== '') {
              const recoveryData = JSON.parse(fileContent);
              ws.send(JSON.stringify({ type: 'espRecoveryState', data: recoveryData }));
              console.log('Sent ESP recovery state to ESP32:', recoveryData);
              
              // Only send separate parameters if ESP recovery state doesn't have them
              // or if frontend has newer parameters
              if (recoveryState.parameters && !recoveryData.parameters) {
                ws.send(JSON.stringify({ type: 'parameters', data: recoveryState.parameters }));
                console.log('Sent additional parameters to ESP32 for recovery:', recoveryState.parameters);
              }
              
              espRecoverySent = true;
            }
          } catch (e) {
            console.error('Failed to send ESP recovery state:', e);
          }
        }
        // Don't return here - let ESP32 messages continue to be processed
      }

      // Handle ESP32 updating its recovery state - accept both message types
      if ((msg.type === 'updateEspRecoveryState' || msg.type === 'espRecoveryState') && isEspClient) {
        espRecoveryState = msg.data || {};
        saveEspRecoveryState();
        console.log('[WS DEBUG] Updated ESP recovery state and saved to ESP_Recovery.json:', espRecoveryState);
        return;
      }

      // Build ESP recovery state from individual ESP32 messages
      if (isEspClient && msg.type) {
        let shouldSave = false;
        
        // Ensure espRecoveryState has the required structure
        if (!espRecoveryState.currentState) espRecoveryState.currentState = 'IDLE';
        if (!espRecoveryState.parameters) espRecoveryState.parameters = {};
        if (!espRecoveryState.timestamp) espRecoveryState.timestamp = new Date().toISOString();
        
        switch (msg.type) {
          case 'currentState':
            if (espRecoveryState.currentState !== msg.value) {
              espRecoveryState.currentState = msg.value;
              espRecoveryState.timestamp = new Date().toISOString();
              shouldSave = true;
              console.log(`[ESP RECOVERY] Updated current state to: ${msg.value}`);
            }
            break;
            
          case 'temperature':
            espRecoveryState.parameters.currentTemperature = msg.value;
            shouldSave = true;
            break;
            
          case 'heatingProgress':
            espRecoveryState.parameters.heatingProgress = msg.value;
            shouldSave = true;
            break;
            
          case 'mixingProgress':
            espRecoveryState.parameters.mixingProgress = msg.value;
            shouldSave = true;
            break;
            
          case 'cycleProgress':
            espRecoveryState.parameters.completedCycles = msg.completed;
            espRecoveryState.parameters.totalCycles = msg.total;
            espRecoveryState.parameters.cycleProgressPercent = msg.percent;
            shouldSave = true;
            break;
            
          case 'syringePercentage':
            espRecoveryState.parameters.syringePercentage = msg.value;
            shouldSave = true;
            break;
        }
        
        if (shouldSave) {
          saveEspRecoveryState();
          console.log(`[ESP RECOVERY] Updated from ${msg.type} message`);
        }
      }

      // Handle heartbeat packets from ESP32
      if (msg.type === 'heartbeat') {
        // Forward heartbeat message to all frontend clients
        for (const client of clients) {
          if (client.readyState === WebSocket.OPEN && !espClients.has(client)) {
            client.send(JSON.stringify({ type: 'heartbeat', from: 'esp32' }));
          }
        }
        // Optionally, you can log the heartbeat
        // console.log('Received heartbeat from ESP32');
        return;
      }

      // Forward ALL ESP32 messages to frontend (unless they're ESP32-only commands)
      if (isEspClient && msg.type && 
          msg.type !== 'updateEspRecoveryState' && 
          msg.type !== 'button' && 
          msg.type !== 'parameters') {
        console.log(`[WS DEBUG] Forwarding ESP32 message to frontend clients: ${msg.type}`);
        // Forward this ESP32 message to all frontend clients
        for (const client of clients) {
          if (client.readyState === WebSocket.OPEN && !espClients.has(client)) {
            console.log(`[WS DEBUG] Sending to frontend client`);
            client.send(JSON.stringify(msg));
          }
        }
      } else if (isEspClient) {
        console.log(`[WS DEBUG] NOT forwarding ESP32 message: ${msg.type} (isEspClient: ${isEspClient})`);
      }

      // If a frontend connects, send recoveryState on request or after identification
      if (msg.type === 'getRecoveryState') {
        ws.send(JSON.stringify({
          type: 'recoveryState',
          data: recoveryState
        }));
      }

      // Handle incoming message types
      if (msg.type === 'temperature' && isEspClient) {
        console.log(`[WS DEBUG] Processing ESP32 temperature: ${msg.value}°C`);
        db.run('INSERT INTO temperature_log (value) VALUES (?)', [msg.value]);
        console.log(`Logged temperature: ${msg.value}°C`);
        
        // Temperature is already forwarded by the general ESP32 message forwarding above
        // No need for duplicate temperatureUpdate messages
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
        // Skip vialSetup buttons as they have their own specific handler below
        if (msg.name !== 'vialSetup') {
          // Forward the button command to all ESP32 clients
          console.log(`Forwarding button command '${msg.name}' to ${espClients.size} ESP32 client(s)`);
          for (const esp of espClients) {
            if (esp.readyState === WebSocket.OPEN) {
              esp.send(JSON.stringify(msg));
              console.log(`Sent button command '${msg.name}' to ESP32 client`);
            } else {
              console.log(`ESP32 client not ready (readyState: ${esp.readyState})`);
            }
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
        
        // Store parameters in recovery state for later ESP32 recovery
        recoveryState.parameters = msg.data;
        saveRecoveryState();
        
        // Update ESP recovery state with all set parameters
        if (msg.data) {
          if (!espRecoveryState.parameters) espRecoveryState.parameters = {};
          
          // Store all the set parameters
          espRecoveryState.parameters.volumeAddedPerCycle = parseFloat(msg.data.volumeAddedPerCycle) || 0;
          espRecoveryState.parameters.syringeDiameter = parseFloat(msg.data.syringeDiameter) || 0;
          espRecoveryState.parameters.desiredHeatingTemperature = parseFloat(msg.data.desiredHeatingTemperature) || 0;
          espRecoveryState.parameters.durationOfHeating = parseFloat(msg.data.durationOfHeating) || 0;
          espRecoveryState.parameters.durationOfMixing = parseFloat(msg.data.durationOfMixing) || 0;
          espRecoveryState.parameters.numberOfCycles = parseInt(msg.data.numberOfCycles) || 0;
          espRecoveryState.parameters.totalCycles = parseInt(msg.data.numberOfCycles) || 0;
          espRecoveryState.parameters.sampleZonesToMix = msg.data.sampleZonesToMix || [];
          
          // Update timestamp
          espRecoveryState.timestamp = new Date().toISOString();
          
          saveEspRecoveryState();
          console.log(`[ESP RECOVERY] Updated with set parameters:`, {
            volumeAddedPerCycle: espRecoveryState.parameters.volumeAddedPerCycle,
            syringeDiameter: espRecoveryState.parameters.syringeDiameter,
            desiredHeatingTemperature: espRecoveryState.parameters.desiredHeatingTemperature,
            durationOfHeating: espRecoveryState.parameters.durationOfHeating,
            durationOfMixing: espRecoveryState.parameters.durationOfMixing,
            numberOfCycles: espRecoveryState.parameters.numberOfCycles,
            totalCycles: espRecoveryState.parameters.totalCycles,
            sampleZonesToMix: espRecoveryState.parameters.sampleZonesToMix
          });
        }
        
        // Forward parameters to all ESP32 clients
        for (const esp of espClients) {
          if (esp.readyState === WebSocket.OPEN) {
            esp.send(JSON.stringify(msg));
          }
        }
      }

      // Handle log cycle button
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
        // Delete frontend recovery file
        if (fs.existsSync(recoveryFile)) {
          fs.unlinkSync(recoveryFile);
          console.log('Frontend_Recovery.json deleted by frontend request.');
        }
        
        // Delete ESP32 recovery file
        if (fs.existsSync(espRecoveryFile)) {
          fs.unlinkSync(espRecoveryFile);
          console.log('ESP_Recovery.json deleted by frontend request.');
        }
        
        // Reset in-memory states
        recoveryState = {};
        espRecoveryState = {};
        
        // Notify all clients of the reset state
        wss.clients.forEach(client => {
          if (client.readyState === WebSocket.OPEN) {
            client.send(JSON.stringify({ type: 'recoveryState', data: recoveryState }));
          }
        });
        
        return;
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

      // Handle state or progress updates from ESP32 - now handled above
      // This block can be removed as all ESP32 messages are forwarded above

      // Reset ESP recovery state when the cycle ends
      if (msg.type === 'button' && msg.name === 'endCycle') {
        resetEspRecoveryState();
      }

    } catch (e) {
      console.error('Bad message:', e);
    }
  });

  ws.on('close', (code, reason) => {
    clients.delete(ws);
    espClients.delete(ws);
    const clientType = isEspClient ? 'ESP32' : 'Frontend';
    console.log(`${clientType} WebSocket client disconnected (code: ${code}, reason: ${reason})`);
    console.log(`Remaining connections: ${clients.size}`);
  });

  ws.on('error', (err) => {
    console.error('WebSocket error:', err);
    clients.delete(ws);
    espClients.delete(ws);
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

server.listen(PORT, '0.0.0.0', () => {
  console.log(`HTTP & WebSocket Server listening on http://0.0.0.0:${PORT}`);
  console.log(`Local access: http://localhost:${PORT}`);
  console.log(`Platform: ${process.platform}`);
});