import React, { useEffect, useState } from 'react';
import 'bulma/css/bulma.min.css';
import useWebSocket from './hooks/useWebSocket';
import './App.css';

// Constants
const TAB_WIDTH = 690;
const INITIAL_PARAMETERS = {
  volumeAddedPerCycle: '',
  durationOfRehydration: '',
  syringeDiameter: '',
  desiredHeatingTemperature: '',
  durationOfHeating: '',
  sampleZonesToMix: [],
  durationOfMixing: '',
  numberOfCycles: '',
};

const parameterFields = [
  { label: "Volume Added Per Cycle (uL)", key: "volumeAddedPerCycle", placeholder: "e.g., 10" },
  { label: "Duration of Rehydration (seconds)", key: "durationOfRehydration", placeholder: "e.g., 30" },
  { label: "Syringe Diameter (in)", key: "syringeDiameter", placeholder: "e.g., 5" },
  { label: "Desired Heating Temperature (°C)", key: "desiredHeatingTemperature", placeholder: "e.g., 90" },
  { label: "Duration of Heating (seconds)", key: "durationOfHeating", placeholder: "e.g., 120" },
  { label: "Duration of Mixing (seconds)", key: "durationOfMixing", placeholder: "e.g., 15" },
  { label: "Number of Cycles", key: "numberOfCycles", placeholder: "e.g., 5" },
];

const buttonConfigs = [
  { id: 'startCycle', label: 'Start Cycle' },
  { id: 'pauseCycle', label: 'Pause Cycle' },
  { id: 'endCycle', label: 'End Cycle' },
  { id: 'extract', label: 'Extract (Pause/Continue)' },
  { id: 'refill', label: 'Refill Syringe (Pause/Continue)' },
  { id: 'logCycle', label: 'Log Cycle' },
];

// Small reusable components for inputs and buttons
function ParameterInput({ label, value, placeholder, onChange }) {
  return (
    <div className="column is-half">
      <div className="field">
        <label className="label">{label}</label>
        <div className="control">
          <input
            type="number"
            className="input is-small"
            placeholder={placeholder}
            value={value}
            onChange={onChange}
            min="0"
          />
        </div>
      </div>
    </div>
  );
}

function ControlButton({ id, label, active, disabled, onClick }) {
  return (
    <button
      className={`button ${active ? 'is-success' : 'is-light'} m-2`}
      disabled={disabled}
      onClick={() => onClick(id)}
    >
      {label}
    </button>
  );
}

function App() {
  // WebSocket hook
  const {
    espOnline,
    recoveryState,
    currentTemp,
    espOutputs,
    sendParameters,
    sendButtonCommand,
    sendRecoveryUpdate,
  } = useWebSocket();

  // Local states
  const [parameters, setParameters] = useState(INITIAL_PARAMETERS);
  const [activeTab, setActiveTab] = useState('parameters');
  const [cycleState, setCycleState] = useState('idle'); // 'idle', 'started', 'paused', 'extract', 'refill'
  const [isPaused, setIsPaused] = useState(false); // Tracks if the cycle is paused
  const [activeButton, setActiveButton] = useState(null); // Tracks the currently active button

  // Handle parameter input change
  const handleParameterChange = (key, value) => {
    if (value === '' || Number(value) > 0) {
      setParameters((prev) => ({ ...prev, [key]: value }));
    }
  };

  // Handle sample zone checkbox changes
  const handleZoneToggle = (zone) => {
    const isSelected = parameters.sampleZonesToMix.includes(zone);
    const updatedZones = isSelected
      ? parameters.sampleZonesToMix.filter((z) => z !== zone)
      : [...parameters.sampleZonesToMix, zone];
    setParameters((prev) => ({ ...prev, sampleZonesToMix: updatedZones }));
  };

  // "Go" button handler
  const handleGoButton = () => {
    sendParameters(parameters);
    setActiveTab('controls');
    setCycleState('idle'); // Reset to idle state
  };

  // Start cycle handler
  const handleStartCycle = () => {
    sendButtonCommand('startCycle', true);
    setCycleState('started'); // Update state to started
    setActiveButton(null); // Reset active button
  };

  // Pause/Resume cycle handler
  const handlePauseCycle = () => {
    if (activeButton === 'pauseCycle') {
      // Resume cycle
      sendButtonCommand('resumeCycle', true);
      setCycleState('started'); // Return to started state
      setIsPaused(false);
      setActiveButton(null); // Reset active button
    } else {
      // Pause cycle
      sendButtonCommand('pauseCycle', true);
      setCycleState('paused'); // Update state to paused
      setIsPaused(true);
      setActiveButton('pauseCycle'); // Set active button to "pauseCycle"
    }
  };

  // End cycle handler
  const handleEndCycle = () => {
    sendButtonCommand('endCycle', true);
    setCycleState('idle'); // Reset to idle state
    setActiveTab('parameters'); // Return to parameters tab
    setActiveButton(null); // Reset active button
  };

  // Extract handler
  const handleExtract = () => {
    if (activeButton === 'extract') {
      setCycleState('started'); // Return to started state
      setActiveButton(null); // Reset active button
    } else {
      sendButtonCommand('extract', true);
      setCycleState('extract'); // Update state to extract
      setActiveButton('extract'); // Set active button to "extract"
    }
  };

  // Refill handler
  const handleRefill = () => {
    if (activeButton === 'refill') {
      setCycleState('started'); // Return to started state
      setActiveButton(null); // Reset active button
    } else {
      sendButtonCommand('refill', true);
      setCycleState('refill'); // Update state to refill
      setActiveButton('refill'); // Set active button to "refill"
    }
  };

  // Button disable logic
  const isButtonDisabled = (id) => {
    if (activeButton && activeButton !== id) return true; // Disable all buttons except the active one
    if (cycleState === 'idle' && id !== 'startCycle') return true; // Only "Start Cycle" is enabled in idle state
    return false; // Enable all other buttons by default
  };

  return (
    <div className="container">
      {/* Recovery State Debug */}
      <section className="box mt-4">
        <h2 className="title is-5">Recovery State (Debug)</h2>
        <pre>{JSON.stringify(recoveryState, null, 2)}</pre>
        <button className="button is-small mt-2" onClick={() => sendRecoveryUpdate({ cycleStatus: 'paused' })}>
          Send Recovery Update
        </button>
      </section>

      <h1 className="title is-2" style={{ marginTop: 0 }}>
        Wet-Dry Cycler Interface
      </h1>

      {/* ESP32 Connection Status */}
      <div className="mb-4">
        <span className="tag is-medium" style={{ backgroundColor: espOnline ? 'green' : 'red' }}></span>
        <span className="ml-2">
          ESP32 Status: <strong>{espOnline ? 'Connected' : 'Disconnected'}</strong>
        </span>
      </div>

      <div className="columns">
        {/* Left: Tabs and Main Content */}
        <div className="column is-three-quarters">
          {/* Tabs */}
          <div className="tabs is-toggle is-fullwidth" style={{ maxWidth: TAB_WIDTH, margin: 0 }}>
            <ul>
              <li className={activeTab === 'parameters' ? 'is-active' : ''}>
                <a
                  onClick={() => setActiveTab('parameters')}
                  style={{ pointerEvents: cycleState !== 'idle' ? 'none' : 'auto', opacity: cycleState !== 'idle' ? 0.5 : 1 }}
                >
                  Set Parameters
                </a>
              </li>
              <li className={activeTab === 'controls' ? 'is-active' : ''}>
                <a
                  onClick={() => setActiveTab('controls')}
                  style={{ pointerEvents: cycleState === 'idle' ? 'none' : 'auto', opacity: cycleState === 'idle' ? 0.5 : 1 }}
                >
                  Controls
                </a>
              </li>
            </ul>
          </div>

          {/* Tab Content */}
          <div className="box" style={{ maxWidth: TAB_WIDTH, margin: 0 }}>
            {activeTab === 'parameters' && (
              <section className="mt-4">
                <div className="columns is-multiline">
                  {parameterFields.map(({ label, key, placeholder }) => (
                    <ParameterInput
                      key={key}
                      label={label}
                      value={parameters[key]}
                      placeholder={placeholder}
                      onChange={(e) => handleParameterChange(key, e.target.value)}
                    />
                  ))}

                  {/* Sample Zones to Mix */}
                  <div className="column is-half">
                    <div className="field">
                      <label className="label">Sample Zones to Mix</label>
                      <div className="control">
                        {['Zone1', 'Zone2', 'Zone3'].map((zone) => (
                          <label key={zone} className="checkbox mr-3">
                            <input
                              type="checkbox"
                              className="mr-2"
                              checked={parameters.sampleZonesToMix.includes(zone)}
                              onChange={() => handleZoneToggle(zone)}
                            />
                            {zone}
                          </label>
                        ))}
                      </div>
                    </div>
                  </div>
                </div>

                <div className="field">
                  <div className="control">
                    <button className="button is-primary" onClick={handleGoButton}>
                      Go
                    </button>
                  </div>
                </div>
              </section>
            )}

            {activeTab === 'controls' && (
              <section className="mt-4">
                <div className="buttons are-medium is-flex is-flex-wrap-wrap is-justify-content-space-between">
                  {buttonConfigs.map(({ id, label }) => (
                    <ControlButton
                      key={id}
                      id={id}
                      label={id === 'pauseCycle' && isPaused ? 'Resume Cycle' : label} // Change label dynamically
                      active={activeButton === id} // Highlight the active button
                      disabled={isButtonDisabled(id)}
                      onClick={(btnId) => {
                        if (btnId === 'startCycle') handleStartCycle();
                        else if (btnId === 'pauseCycle') handlePauseCycle();
                        else if (btnId === 'endCycle') handleEndCycle();
                        else if (btnId === 'extract') handleExtract();
                        else if (btnId === 'refill') handleRefill();
                        else sendButtonCommand(btnId, true);
                      }}
                    />
                  ))}
                </div>
              </section>
            )}
          </div>
        </div>

        {/* Right: ESP32 Outputs */}
        <div className="column is-one-quarter">
          <div className="box" style={{ maxWidth: 400, margin: 0 }}>
            <h2 className="title is-5">ESP32 Outputs</h2>
            <div className="columns is-full is-multiline">
              {/* Temperature */}
              <div className="column is-full">
                <label className="label is-small">Temperature Data</label>
                <input
                  type="text"
                  className="input is-small"
                  value={currentTemp !== null ? `${currentTemp} °C` : 'N/A'}
                  readOnly
                />
              </div>

              {/* Syringe Limit */}
              <div className="column is-full">
                <label className="label is-small">Syringe Limit</label>
                <progress className="progress is-primary is-small" value={espOutputs.syringeLimit} max="100">
                  {espOutputs.syringeLimit}%
                </progress>
              </div>

              {/* Extraction Ready */}
              <div className="column is-full">
                <label className="label is-small">Extraction Ready</label>
                <span
                  className="tag is-medium"
                  style={{
                    backgroundColor: espOutputs.extractioReady === 'ready' ? 'green' : 'red',
                    color: 'white',
                  }}
                >
                  {espOutputs.extractioReady === 'ready' ? 'Extraction Ready' : 'Not Ready'}
                </span>
              </div>

              {/* Cycles Completed */}
              <div className="column is-half">
                <label className="label is-small"># Cycles Completed</label>
                <input type="text" className="input is-small" value={espOutputs.cyclesCompleted} readOnly />
              </div>

              {/* Cycle Progress */}
              <div className="column is-half">
                <label className="label is-small">Cycle Progress</label>
                <progress className="progress is-info is-small" value={espOutputs.cycleProgress} max="100">
                  {espOutputs.cycleProgress}%
                </progress>
              </div>

              {/* Syringe Used */}
              <div className="column is-full">
                <label className="label is-small">Syringe Used</label>
                <input type="text" className="input is-small" value={espOutputs.syringeUsed} readOnly />
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

export default App;