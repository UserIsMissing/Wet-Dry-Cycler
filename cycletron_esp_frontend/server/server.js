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

// ESP Recovery file write debouncing
let espRecoveryWriteTimeout = null;
let pendingEspRecoveryWrite = false;

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
// DISABLED: File I/O causes ESP32 disconnections on Windows
// Will keep ESP recovery state in memory only for now
/*
if (fs.existsSync(espRecoveryFile)) {
  try {
    espRecoveryState = JSON.parse(fs.readFileSync(espRecoveryFile));
    console.log("Loaded ESP recovery state:", espRecoveryState);
  } catch (e) {
    console.error("Failed to load ESP recovery state file:", e);
    espRecoveryState = {};
  }
}
*/

function saveRecoveryState() {
  // Use async file write to prevent blocking the event loop
  fs.writeFile(recoveryFile, JSON.stringify(recoveryState, null, 2), (err) => {
    if (err) {
      console.error('Failed to save recovery state:', err);
    }
  });
}

// Function to save ESP recovery state - MEMORY ONLY (File I/O disabled to prevent ESP32 disconnections)
function saveEspRecoveryStateDebounced() {
  // DISABLED: File I/O causes ESP32 disconnections on Windows
  // Keeping function for consistency but only tracking in memory
  console.log('ESP recovery state updated (memory only):', espRecoveryState);
  
  // NEW: Save to SQLite database instead of JSON file
  saveEspRecoveryStateToDatabase();
  
  // TODO: Implement alternative storage solution (SQLite database) when file I/O issue is resolved
  /*
  // Cancel any pending write
  if (espRecoveryWriteTimeout) {
    clearTimeout(espRecoveryWriteTimeout);
  }
  
  // Mark that we have a pending write
  pendingEspRecoveryWrite = true;
  
  // Schedule the write to happen after 2 seconds of no more state changes
  espRecoveryWriteTimeout = setTimeout(() => {
    if (pendingEspRecoveryWrite) {
      // Use async file write to prevent blocking the event loop
      fs.writeFile(espRecoveryFile, JSON.stringify(espRecoveryState, null, 2), (err) => {
        if (err) {
          console.error('Failed to save ESP recovery state:', err);
        } else {
          console.log('ESP recovery state saved (debounced):', espRecoveryState);
        }
        pendingEspRecoveryWrite = false;
      });
    }
  }, 2000); // 2 second delay
  */
}

// Save ESP recovery state to SQLite database (non-blocking)
function saveEspRecoveryStateToDatabase() {
  if (Object.keys(espRecoveryState).length === 0) return; // Don't save empty state
  
  const currentState = espRecoveryState.currentState || 'unknown';
  const dataJson = JSON.stringify(espRecoveryState);
  
  // Use INSERT OR REPLACE to upsert (update or insert) with id=1
  db.run(
    'INSERT OR REPLACE INTO esp_recovery_state (id, current_state, data) VALUES (1, ?, ?)',
    [currentState, dataJson],
    function(err) {
      if (err) {
        console.error('Failed to save ESP recovery state to database:', err);
      } else {
        console.log('ESP recovery state saved to database:', currentState);
      }
    }
  );
}

// Load ESP recovery state from SQLite database
function loadEspRecoveryStateFromDatabase() {
  db.get(
    'SELECT current_state, data, timestamp FROM esp_recovery_state WHERE id = 1',
    (err, row) => {
      if (err) {
        console.error('Failed to load ESP recovery state from database:', err);
        return;
      }
      
      if (row) {
        try {
          espRecoveryState = JSON.parse(row.data);
          console.log('Loaded ESP recovery state from database:', espRecoveryState);
          console.log('Last updated:', row.timestamp);
        } catch (parseErr) {
          console.error('Failed to parse ESP recovery state from database:', parseErr);
          espRecoveryState = {};
        }
      } else {
        console.log('No ESP recovery state found in database');
        espRecoveryState = {};
      }
    }
  );
}

// Send ESP recovery state to a newly connected ESP32 client
function sendEspRecoveryStateToClient(espClient) {
  // Load the latest state from database first
  db.get(
    'SELECT current_state, data, timestamp FROM esp_recovery_state WHERE id = 1',
    (err, row) => {
      if (err) {
        console.error('Failed to load ESP recovery state for new client:', err);
        return;
      }
      
      if (row && row.data) {
        try {
          const savedState = JSON.parse(row.data);
          console.log(`[ESP RECOVERY] Sending saved state to newly connected ESP32:`, savedState);
            // Send recovery state to ESP32
          if (espClient.readyState === WebSocket.OPEN) {
            espClient.send(JSON.stringify({
              type: 'espRecoveryState',
              data: savedState
            }));
            console.log(`[ESP RECOVERY] Recovery state sent to ESP32: ${savedState.currentState}`);
          }
        } catch (parseErr) {
          console.error('Failed to parse ESP recovery state for new client:', parseErr);
        }
      } else {
        console.log('[ESP RECOVERY] No saved state found for new ESP32 client');
      }
    }
  );
}

// ----------------- SQLite DB Setup -----------------
const db = new sqlite3.Database('esp_data.db');
db.run(`CREATE TABLE IF NOT EXISTS temperature_log (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
  value REAL
)`);

// Create ESP recovery state table
db.run(`CREATE TABLE IF NOT EXISTS esp_recovery_state (
  id INTEGER PRIMARY KEY,
  current_state TEXT,
  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
  data TEXT
)`, () => {
  // Load ESP recovery state from database after table is ensured to exist
  loadEspRecoveryStateFromDatabase();
});

// ----------------- API Routes -----------------
app.get('/api/history', (req, res) => {
  db.all('SELECT * FROM temperature_log ORDER BY id DESC LIMIT 100', (err, rows) => {
    if (err) return res.status(500).json({ error: err });
    res.json(rows.reverse());
  });
});

// Add this route to reset the recovery state
app.post('/api/resetRecoveryState', (req, res) => {
  // Delete frontend recovery state (async)
  if (fs.existsSync(recoveryFile)) {
    fs.unlink(recoveryFile, (err) => {
      if (err) console.error('Failed to delete frontend recovery file:', err);
      else console.log('Frontend_Recovery.json deleted by request.');
    });
  }  // Delete ESP32 recovery state (async)
  if (fs.existsSync(espRecoveryFile)) {
    fs.unlink(espRecoveryFile, (err) => {
      if (err) console.error('Failed to delete ESP recovery file:', err);
      else console.log('ESP_Recovery.json deleted by request.');
    });
  }
  
  // Clear ESP recovery state from database
  db.run('DELETE FROM esp_recovery_state WHERE id = 1', (err) => {
    if (err) console.error('Failed to clear ESP recovery state from database:', err);
    else console.log('ESP recovery state cleared from database.');
  });
  
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

  const clientIP = req.socket.remoteAddress || 'unknown';
  const clientPort = req.socket.remotePort || 'unknown';
  console.log(`New WebSocket connection from ${clientIP}:${clientPort}`);

  if (!clients.has(ws)) {
    clients.add(ws);
    console.log(`Total WebSocket connections: ${clients.size}`);
  }

  ws.on('message', (message) => {
    console.log('[WS DEBUG] Received message:', message);
    
    // Use setImmediate to process message in next tick, preventing blocking
    setImmediate(() => {
      try {
        const msg = JSON.parse(message);

        // Debug: log all message types
        if (msg.type) {
          console.log(`[WS DEBUG] Message type: ${msg.type}`);
        }      // Detect ESP32 by a message property (e.g., type === 'esp_announce')
      if (msg.from === 'esp32') {
        if (!isEspClient) {
          console.log(`[WS DEBUG] NEW ESP32 CLIENT DETECTED! Adding to espClients set.`);
          isEspClient = true;
          espClients.add(ws);
          
          // Send recovery state to newly connected ESP32 if available
          sendEspRecoveryStateToClient(ws);
        }
        // Don't return here - let ESP32 messages continue to be processed
      }      // DEBOUNCED ESP Recovery: Only track state changes from ESP32 (with delayed file writes)
      if (msg.type === 'currentState' && isEspClient) {
        const newState = msg.value;
        
        // Only update in memory if the state actually changed
        if (!espRecoveryState.currentState || espRecoveryState.currentState !== newState) {
          // Preserve existing parameters when updating state
          const existingParameters = espRecoveryState.parameters || {};
          
          espRecoveryState = {
            currentState: newState,
            timestamp: new Date().toISOString(),
            parameters: existingParameters  // Keep existing parameters
          };
          
          // Use debounced file saving to prevent frequent I/O
          saveEspRecoveryStateDebounced();
          console.log(`[ESP RECOVERY] State changed to: ${newState} (will save after delay)`);
        }
        // Continue processing the message normally
      }// Handle heartbeat packets from ESP32
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
          msg.type !== 'button' && 
          msg.type !== 'parameters') {
        console.log(`[WS DEBUG] Forwarding ESP32 message to frontend clients: ${msg.type}`);
        // Forward this ESP32 message to all frontend clients
        for (const client of clients) {
          if (client.readyState === WebSocket.OPEN && !espClients.has(client)) {
            try {
              console.log(`[WS DEBUG] Sending to frontend client`);
              client.send(JSON.stringify(msg));
            } catch (sendError) {
              console.error(`[WS ERROR] Failed to send message to frontend client:`, sendError);
              // Remove broken client
              clients.delete(client);
            }
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
        if (msg.name !== 'vialSetup') {        // Forward the button command to all ESP32 clients
        console.log(`Forwarding button command '${msg.name}' to ${espClients.size} ESP32 client(s)`);
        for (const esp of espClients) {
          if (esp.readyState === WebSocket.OPEN) {
            try {
              esp.send(JSON.stringify(msg));
              console.log(`Sent button command '${msg.name}' to ESP32 client`);
            } catch (sendError) {
              console.error(`Failed to send button command to ESP32:`, sendError);
              // Remove broken ESP client
              espClients.delete(esp);
              clients.delete(esp);
            }
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
      }      if (msg.type === 'parameters') {
        console.log('Received parameters:', msg.data);
        
        // Store parameters in recovery state for later ESP32 recovery
        recoveryState.parameters = msg.data;
        saveRecoveryState();
        
        // Also store parameters in ESP recovery state for ESP32 recovery
        if (!espRecoveryState.parameters) {
          espRecoveryState.parameters = {};
        }
        espRecoveryState.parameters = msg.data;
        saveEspRecoveryStateToDatabase();
        console.log('[ESP RECOVERY] Parameters stored for ESP32 recovery');
        
        // Forward parameters to all ESP32 clients
        for (const esp of espClients) {
          if (esp.readyState === WebSocket.OPEN) {
            try {
              esp.send(JSON.stringify(msg));
            } catch (sendError) {
              console.error(`Failed to send parameters to ESP32:`, sendError);
              // Remove broken ESP client
              espClients.delete(esp);
              clients.delete(esp);
            }
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

        // Write the entry to a new file (async to prevent blocking)
        fs.writeFile(logFile, JSON.stringify(entry, null, 2), (err) => {
          if (err) {
            console.error('Failed to write log file:', err);
          } else {
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
        });
      }      if (msg.type === 'resetRecoveryState') {
        // Delete frontend recovery file (async)
        if (fs.existsSync(recoveryFile)) {
          fs.unlink(recoveryFile, (err) => {
            if (err) console.error('Failed to delete frontend recovery file:', err);
            else console.log('Frontend_Recovery.json deleted by frontend request.');
          });
        }        // Delete ESP32 recovery file (async) - RE-ENABLED for user-initiated reset
        // This is safe because it's a manual user action, not automatic ESP32 state changes
        if (fs.existsSync(espRecoveryFile)) {
          fs.unlink(espRecoveryFile, (err) => {
            if (err) console.error('Failed to delete ESP recovery file:', err);
            else console.log('ESP_Recovery.json deleted by frontend request.');
          });
        }
        
        // Clear ESP recovery state from database
        db.run('DELETE FROM esp_recovery_state WHERE id = 1', (err) => {
          if (err) console.error('Failed to clear ESP recovery state from database:', err);
          else console.log('ESP recovery state cleared from database by frontend request.');
        });
        
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
            try {
              esp.send(JSON.stringify({ type: 'vialSetup', status: msg.status }));
            } catch (sendError) {
              console.error(`Failed to send vial setup to ESP32:`, sendError);
              // Remove broken ESP client
              espClients.delete(esp);
              clients.delete(esp);
            }
          }
        }
      }      // Handle state or progress updates from ESP32 - now handled above
      // This block can be removed as all ESP32 messages are forwarded above

      // No more ESP recovery state reset on endCycle since we removed ESP recovery

    } catch (e) {
      console.error('Bad message:', e);
    }
    }); // End setImmediate
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