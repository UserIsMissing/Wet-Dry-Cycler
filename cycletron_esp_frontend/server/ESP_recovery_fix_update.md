# ESP Recovery Fix Update

## Overview
This document summarizes the successful resolution of ESP32 disconnection issues and implementation of robust ESP recovery functionality for the Wet-Dry Cycler system.

## Problem Statement
The ESP32 S3 was experiencing frequent WebSocket disconnections when the server attempted to save ESP recovery state to JSON files on Windows. This prevented reliable operation and made recovery functionality unusable.

## Root Cause Analysis
- **Primary Issue**: File I/O operations (`fs.writeFileSync()` and `fs.writeFile()`) were blocking the Node.js event loop on Windows
- **Secondary Issue**: ESP32 parameter parsing only handled string types, but frontend was sending numeric types
- **Result**: WebSocket timeouts and connection drops, plus parameter parsing failures

## Solution Implementation

### 1. Replaced File I/O with SQLite Database
**Before:**
```javascript
// ESP recovery state saved to ESP_Recovery.json file
fs.writeFile(espRecoveryFile, JSON.stringify(espRecoveryState, null, 2), callback);
```

**After:**
```javascript
// ESP recovery state saved to SQLite database
db.run('INSERT OR REPLACE INTO esp_recovery_state (id, current_state, data) VALUES (1, ?, ?)', 
       [currentState, dataJson], callback);
```

**Why it works:** SQLite operations are non-blocking and don't cause the same I/O bottlenecks that JSON file writes were causing on Windows.

### 2. Enhanced ESP32 Parameter Type Handling
**Before (ESP32 code):**
```cpp
// Only handled string parameters
volumeAddedPerCycle = parameters["volumeAddedPerCycle"].is<const char*>() ? 
    atof(parameters["volumeAddedPerCycle"].as<const char*>()) : 0.0;
```

**After (ESP32 code):**
```cpp
// Handles both string and numeric parameters
float value = 0.0;
if (parameters["volumeAddedPerCycle"].is<const char*>()) {
    value = atof(parameters["volumeAddedPerCycle"].as<const char*>());
} else if (parameters["volumeAddedPerCycle"].is<float>() || parameters["volumeAddedPerCycle"].is<int>()) {
    value = parameters["volumeAddedPerCycle"].as<float>();
}
volumeAddedPerCycle = value;
```

**Why it works:** ESP32 can now handle parameters regardless of whether they come as strings (`"7"`) or numbers (`7`) from different sources.

### 3. Comprehensive Recovery State Management
**Server-side improvements:**
- Tracks ESP32 state changes via `currentState` messages from ESP32
- Stores both `currentState` and `parameters` in recovery state
- Automatically sends complete recovery state to newly connected ESP32 clients
- Uses debounced database writes to prevent excessive I/O operations

**ESP32-side improvements:**
- Properly parses recovery packets with both string and numeric parameter types
- Restores complete system state including all parameters and progress tracking
- Validates recovery data structure before applying state changes

### 4. Preserved User-Initiated Reset Functionality
- ESP_Recovery.json file deletion is still enabled for manual reset button
- Database clearing for ESP recovery state on user request
- Safe because it's user-initiated, not automatic ESP32 state changes

## Technical Implementation Details

### Database Schema
```sql
CREATE TABLE esp_recovery_state (
  id INTEGER PRIMARY KEY,
  current_state TEXT,
  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
  data TEXT
);
```

### ESP32 Recovery Message Format
```json
{
  "type": "espRecoveryState",
  "data": {
    "currentState": "HEATING",
    "timestamp": "2025-06-06T00:25:13.946Z",
    "parameters": {
      "volumeAddedPerCycle": 7,
      "syringeDiameter": 7,
      "desiredHeatingTemperature": 7,
      "durationOfHeating": 7,
      "sampleZonesToMix": [1, 3],
      "durationOfMixing": 7,
      "numberOfCycles": 7
    }
  }
}
```

### Key Server Functions
- `saveEspRecoveryStateToDatabase()` - Saves ESP state to SQLite
- `loadEspRecoveryStateFromDatabase()` - Loads ESP state from SQLite on startup
- `sendEspRecoveryStateToClient()` - Sends recovery state to newly connected ESP32

## Results

### ✅ Issues Resolved
- **No more ESP32 disconnections** - SQLite operations don't block the event loop
- **Reliable state recovery** - ESP32 maintains state through restarts/reconnections
- **Proper parameter handling** - All parameters restored with correct numeric values
- **Seamless operation continuity** - System continues from exactly where it left off
- **Manual reset capability** - User can still manually reset recovery state when needed

### 📊 Performance Improvements
- WebSocket connections remain stable during state changes
- Database operations are non-blocking and efficient
- Recovery state loading happens automatically on ESP32 connection
- Parameter type flexibility prevents parsing errors

## Testing Verification
- ESP32 restart during HEATING state → Successfully restored to HEATING with all parameters
- Frontend parameter updates → Properly stored and recovered with correct numeric types
- Multiple disconnect/reconnect cycles → No data loss or connection issues
- Manual reset functionality → Cleanly clears all recovery state

## Files Modified
1. **Server:** `cycletron_esp_frontend/server/server.js`
   - Added SQLite database integration
   - Enhanced ESP32 client detection and recovery state management
   - Improved parameter storage and forwarding

2. **ESP32:** `Cycletron/src/handle_functions.cpp`
   - Updated `handleParametersPacket()` for flexible type handling
   - Enhanced `handleRecoveryPacket()` for robust parameter restoration
   - Added support for both string and numeric parameter types

## Future Considerations
- The SQLite approach can be extended for other persistent data storage needs
- Recovery state could include additional system metrics if needed
- Database could be backed up periodically for additional redundancy

## Conclusion
This solution provides robust ESP recovery functionality without the Windows-specific file I/O issues that were causing the original problems. The system now maintains complete state persistence across ESP32 restarts while ensuring stable WebSocket connections.
