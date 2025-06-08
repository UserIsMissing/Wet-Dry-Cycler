import React, { useEffect, useState, useRef } from 'react';
import 'bulma/css/bulma.min.css';
import { useWebSocket } from './context/WebSocketContext';
import './App.css';

// Constants
const TAB_WIDTH = 690;
const INITIAL_PARAMETERS = {
  volumeAddedPerCycle: '',
  syringeDiameter: '',
  desiredHeatingTemperature: '',
  durationOfHeating: '',
  sampleZonesToMix: [],
  durationOfMixing: '',
  numberOfCycles: '',
};

const parameterFields = [
  { label: "Volume Added Per Cycle (uL)", key: "volumeAddedPerCycle", placeholder: "e.g., 10" },
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

function ControlButton({ id, label, active, disabled, onClick, isPaused }) {
  const buttonLabel = id === 'pauseCycle' && isPaused ? 'Resume Cycle' : label;
  const buttonClass = id === 'pauseCycle' && isPaused ? 'is-success' : active ? 'is-success' : 'is-light';

  return (
    <button
      className={`button ${buttonClass} m-2`}
      disabled={disabled}
      onClick={() => onClick(id)}
    >
      {buttonLabel}
    </button>
  );
}

function App() {
  const {
    espOnline,
    recoveryState,
    currentTemp,
    espOutputs,
    setEspOutputs,
    sendParameters,
    sendButtonCommand,
    sendRecoveryUpdate,
    resetRecoveryState,
  } = useWebSocket();

  const [parameters, setParameters] = useState(INITIAL_PARAMETERS);
  const [activeTab, setActiveTab] = useState('parameters');
  const [cycleState, setCycleState] = useState('idle');
  const [activeButton, setActiveButton] = useState(null);
  const [isPaused, setIsPaused] = useState(false);
  const [showVialSetup, setShowVialSetup] = useState(true);
  const [vialSetupStep, setVialSetupStep] = useState('prompt');
  const [serverIP, setServerIP] = useState('localhost:5175');

  // Fetch server IP on component mount
  useEffect(() => {
    fetch('/api/serverIP')
      .then((res) => res.json())
      .then((data) => {
        if (data && data.serverAddress) {
          setServerIP(data.serverAddress);
        }
      })
      .catch((err) => {
        console.error('Failed to fetch server IP:', err);
      });
  }, []);

  // Restore UI state from recoveryState
  useEffect(() => {
    if (recoveryState) {
      setParameters(recoveryState.parameters || INITIAL_PARAMETERS);
      setActiveTab(recoveryState.activeTab || 'parameters');
      setCycleState(recoveryState.cycleState || 'idle');
      setActiveButton(recoveryState.activeButton || null);
      setVialSetupStep(
        recoveryState.vialSetupStep !== undefined
          ? recoveryState.vialSetupStep
          : 'prompt'
      );
      // Restore isPaused based on cycleState or activeButton
      setIsPaused(
        recoveryState.cycleState === 'paused' ||
        recoveryState.activeButton === 'pauseCycle'
      );
    }
  }, [recoveryState]);

  // Whenever vialSetupStep changes, persist it in recovery state
  useEffect(() => {
    if (recoveryState && recoveryState.vialSetupStep !== vialSetupStep) {
      sendRecoveryUpdate({ vialSetupStep });
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [vialSetupStep]);

  // Listen for ESP32 endOfCycles events
  useEffect(() => {
    const handleEspEndOfCycles = (event) => {
      console.log('Received ESP32 endOfCycles event:', event.detail);
      // Call handleLogCycle and then handleEndCycle directly
      // Exclude extractionReady from espOutputs (syringeLeft is calculated, so no need to exclude)
      const { extractionReady, ...filteredEspOutputs } = espOutputs;
      sendButtonCommand('logCycle', true, {
        parameters,
        espOutputs: filteredEspOutputs,
        timestamp: new Date().toISOString(),
      });
      
      // Wait a moment for the log to complete, then end the cycle
      setTimeout(() => {
        sendButtonCommand('endCycle', true); // Send 'on' to the server
        setCycleState('idle'); // Reset the cycle state to 'idle'
        setActiveButton(null); // Reset the active button
        setIsPaused(false); // Ensure the paused state is reset
        sendRecoveryUpdate({
          parameters,
          machineStep: 'idle',
          cycleState: 'idle',
          lastAction: 'endCycle',
          progress: 0,
          activeTab: 'parameters',
        });
        setVialSetupStep('prompt'); // Reset the vial setup step
        setShowVialSetup(true); // Show the vial setup prompt again
        setActiveTab('parameters'); // Switch to the "Set Parameters" tab
      }, 500);
    };

    window.addEventListener('espEndOfCycles', handleEspEndOfCycles);

    return () => {
      window.removeEventListener('espEndOfCycles', handleEspEndOfCycles);
    };
  }, [espOutputs, parameters, sendButtonCommand, sendRecoveryUpdate, setCycleState, setActiveButton, setIsPaused, setVialSetupStep, setShowVialSetup, setActiveTab]);

  const handleParameterChange = (key, value) => {
    if (value === '' || Number(value) > 0) {
      setParameters((prev) => ({ ...prev, [key]: value }));
    }
  };

  const handleZoneToggle = (zone) => {
    const isSelected = parameters.sampleZonesToMix.includes(zone);
    const updatedZones = isSelected
      ? parameters.sampleZonesToMix.filter((z) => z !== zone)
      : [...parameters.sampleZonesToMix, zone];
    setParameters((prev) => ({ ...prev, sampleZonesToMix: updatedZones }));
  };

  const handleGoButton = () => {
    sendParameters(parameters); // Send parameters to the ESP32
    sendRecoveryUpdate({
      // ...parameters, // Add all parameters to recovery state
      parameters,
      machineStep: 'idle',
      lastAction: 'setParameters',
      progress: 0,
      activeTab: 'controls', // Update the active tab in recovery state
    });
    setCycleState('idle');
    setActiveTab('controls'); // Switch to the controls tab
  };

  const handleStartCycle = () => {
    sendButtonCommand('startCycle', true);
    setCycleState('started');
    setActiveButton(null);
    sendRecoveryUpdate({
      machineStep: 'started',
      cycleState: 'started', // <-- important!
      lastAction: 'startCycle',
      progress: 0,
    });
  };

  const handlePauseCycle = () => {
    const isPausing = !isPaused;
    sendButtonCommand('pauseCycle', isPausing);
    setCycleState(isPausing ? 'paused' : 'started');
    setIsPaused(isPausing);
    setActiveButton(isPausing ? 'pauseCycle' : null);
    sendRecoveryUpdate({
      parameters,
      machineStep: isPausing ? 'paused' : 'started',
      cycleState: isPausing ? 'paused' : 'started',
      lastAction: isPausing ? 'pauseCycle' : 'started',
      activeButton: isPausing ? 'pauseCycle' : null,
      activeTab,
    });
  };

  const handleEndCycle = () => {
    // Automatically log the cycle before ending
    handleLogCycle();
    
    sendButtonCommand('endCycle', true); // Send 'on' to the server
    setCycleState('idle'); // Reset the cycle state to 'idle'
    setActiveButton(null); // Reset the active button
    setIsPaused(false); // Ensure the paused state is reset
    sendRecoveryUpdate({
      // ...parameters,
      parameters, // <-- add this line
      machineStep: 'idle',
      cycleState: 'idle',
      lastAction: 'endCycle',
      progress: 0,
      activeTab: 'parameters',
    });
    // setParameters(INITIAL_PARAMETERS); // Reset parameters to initial state
    setVialSetupStep('prompt'); // Reset the vial setup step
    setShowVialSetup(true); // Show the vial setup prompt again
    setActiveTab('parameters'); // Switch to the "Set Parameters" tab
  };

  const handleExtract = () => {
    const isCanceling = activeButton === 'extract';
    sendButtonCommand('extract', !isCanceling); // send "on" if starting, "off" if canceling
    setCycleState(isCanceling ? 'started' : 'extract');
    setActiveButton(isCanceling ? null : 'extract');
    
    // When Extract is clicked the first time (starting), set extraction ready to 'N/A' (waiting)
    // When Extract is clicked the second time (canceling), also set extraction ready back to 'N/A'
    if (!isCanceling) {
      // Starting extraction - set to waiting state
      setEspOutputs((prev) => ({
        ...prev,
        extractionReady: 'N/A'
      }));
    } else {
      // Canceling extraction - reset to default state
      setEspOutputs((prev) => ({
        ...prev,
        extractionReady: 'N/A'
      }));
    }
    
    sendRecoveryUpdate({
      parameters,
      machineStep: isCanceling ? 'started' : 'extract',
      cycleState: isCanceling ? 'started' : 'extract',
      lastAction: isCanceling ? 'started' : 'extract',
      activeButton: isCanceling ? null : 'extract',
      activeTab,
    });
  };

  const handleRefill = () => {
    const isCanceling = activeButton === 'refill';
    sendButtonCommand('refill', !isCanceling); // send "on" if starting, "off" if canceling
    setCycleState(isCanceling ? 'started' : 'refill');
    setActiveButton(isCanceling ? null : 'refill');
    sendRecoveryUpdate({
      parameters,
      machineStep: isCanceling ? 'started' : 'refill',
      cycleState: isCanceling ? 'started' : 'refill',
      lastAction: isCanceling ? 'started' : 'refill',
      activeButton: isCanceling ? null : 'refill',
      activeTab,
    });
  };

  const handleLogCycle = () => {
    // Exclude extractionReady from espOutputs (syringeLeft is calculated, so no need to exclude)
    const { extractionReady, ...filteredEspOutputs } = espOutputs;
    sendButtonCommand('logCycle', true, {
      parameters,
      espOutputs: filteredEspOutputs,
      timestamp: new Date().toISOString(),
    });
  };

  const isButtonDisabled = (id) => {
    if (id === 'startCycle' && cycleState !== 'idle') return true;
    if (id === 'endCycle' && cycleState === 'idle') return true;
    if (['pauseCycle', 'extract', 'refill'].includes(id) && cycleState === 'idle') return true;

    if (cycleState === 'paused' && id !== 'pauseCycle') return true;

    // Extract button is disabled when it's active (green) but extraction is not ready yet
    if (id === 'extract' && activeButton === 'extract' && espOutputs.extractionReady !== 'ready') return true;

    if (activeButton && activeButton !== id) return true;
    if (cycleState === 'idle' && id !== 'startCycle') return true;
    return false;
  };

  // When closing the overlay, update recovery state
  const handleVialSetupStep = (step) => {
    setVialSetupStep(step);
    sendRecoveryUpdate({ vialSetupStep: step });

    // Send vial setup packet to ESP32
    if (step === 'continue') {
      // First dialog: "Yes" to needing vial preparation
      sendButtonCommand('vialSetup', true, { state: 'yes' }); // Changed status to state
    } else if (step === null) {
      // This could be either "No" from first dialog or "Continue" from second dialog
      if (vialSetupStep === 'prompt') {
        // Coming from first dialog: "No" to vial preparation
        sendButtonCommand('vialSetup', true, { state: 'no' }); // Changed status to state
      } else {
        // Coming from second dialog: "Continue" after vial setup
        sendButtonCommand('vialSetup', true, { state: 'continue' }); // Changed status to state
      }
    }
  };

  // Function to reset both frontend and ESP recovery state and restart ESP32
  const resetRecoveryData = () => {
    // Reset both frontend and ESP recovery state using the single endpoint
    fetch('/api/resetRecoveryState', { method: 'POST' })
      .then((res) => res.json())
      .then((data) => {
        if (data.success) {
          console.log('Both Frontend_Recovery.json and ESP_Recovery.json reset');
          // After successfully resetting recovery data, restart the ESP32
          sendButtonCommand('restartESP32', true);
          console.log('ESP32 restart command sent');
          // Wait 2 seconds to ensure the restart command reaches the ESP32 before reloading
          setTimeout(() => {
            window.location.reload();
          }, 2000);
        }
      })
      .catch((err) => console.error('Failed to reset recovery state:', err));
  };

  // Handler for endOfCycles: log cycle, then end cycle
  const handleEndOfCycles = () => {
    handleLogCycle();
    setTimeout(() => {
      handleEndCycle();
    }, 500); // 500ms delay to ensure log finishes
  };

  // Store the last valid temperature value
  const lastTempRef = useRef(null);
  useEffect(() => {
    if (typeof currentTemp === 'number' && !isNaN(currentTemp)) {
      lastTempRef.current = currentTemp;
    }
  }, [currentTemp]);

  return (
    <div className="container" style={{ position: 'relative' }}>
      <h1 className="title is-2">Wet-Dry Cycler Interface</h1>
      
      <div className="mb-3">
        <span className="has-text-weight-semibold">Server IP: </span>
        <span className="has-text-link">{serverIP}</span>
      </div>

      <div className="mb-4">
        <span className="tag is-medium" style={{ backgroundColor: espOnline ? 'green' : 'red' }}></span>
        <span className="ml-2">
          ESP32 Status: <strong>{espOnline ? 'Connected' : 'Disconnected'}</strong>
        </span>
      </div>

      <div className="columns">
        <div className="column is-three-quarters" style={{ position: 'relative' }}>
          {/* Vial Setup Overlay - now only covers the tabs/box area */}
          {vialSetupStep && (
            <div
              style={{
                position: 'absolute',
                zIndex: 1000,
                top: 0,
                left: 0,
                width: TAB_WIDTH,
                maxWidth: '100%',
                height: '100%',
                background: 'rgba(255,255,255,0.98)',
                display: 'flex',
                flexDirection: 'column',
                alignItems: 'center',
                justifyContent: 'center',
                pointerEvents: 'auto',
              }}
            >
              <h1 className="title is-2 mb-5">Vial Setup</h1>
              {vialSetupStep === 'prompt' ? (
                <>
                  <h2 className="title is-4 mb-5">Need to Prepare Vials?</h2>
                  <div>
                    <button
                      className="button is-primary is-large mr-4"
                      style={{ fontSize: '2rem', padding: '2rem 4rem' }}
                      onClick={() => handleVialSetupStep('continue')}
                    >
                      Yes
                    </button>
                    <button
                      className="button is-light is-large"
                      style={{ fontSize: '2rem', padding: '2rem 4rem' }}
                      onClick={() => handleVialSetupStep(null)}
                    >
                      No
                    </button>
                  </div>
                </>
              ) : (
                <button
                  className="button is-primary is-large"
                  style={{ fontSize: '2rem', padding: '2rem 4rem' }}
                  onClick={() => handleVialSetupStep(null)}
                >
                  Continue
                </button>
              )}
            </div>
          )}

          <div className="tabs is-toggle is-fullwidth" style={{ maxWidth: TAB_WIDTH }}>
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

          <div className="box" style={{ maxWidth: TAB_WIDTH }}>
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

                  <div className="column is-half">
                    <div className="field">
                      <label className="label">Sample Zones to Mix</label>
                      <div className="control">
                        {[1, 2, 3].map((zone) => (
                          <label key={zone} className="checkbox mr-3">
                            <input
                              type="checkbox"
                              className="mr-2"
                              checked={parameters.sampleZonesToMix.includes(zone)}
                              onChange={() => handleZoneToggle(zone)}
                            />
                            {`Zone${zone}`}
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
                      label={id === 'pauseCycle' && isPaused ? 'Resume Cycle' : label}
                      active={activeButton === id}
                      disabled={isButtonDisabled(id)}
                      onClick={(btnId) => {
                        if (btnId === 'startCycle') handleStartCycle();
                        else if (btnId === 'pauseCycle') handlePauseCycle();
                        else if (btnId === 'endCycle') handleEndCycle();
                        else if (btnId === 'extract') handleExtract();
                        else if (btnId === 'refill') handleRefill();
                        else if (btnId === 'logCycle') handleLogCycle();
                        else sendButtonCommand(btnId);
                      }}
                      isPaused={isPaused} // Pass the isPaused state
                    />
                  ))}
                </div>
              </section>
            )}
          </div>
        </div>

        <div className="column is-one-quarter">
          <div className="box" style={{ maxWidth: 400 }}>
            <h2 className="title is-5">ESP32 Outputs</h2>
            <div className="columns is-full is-multiline">
              <div className="column is-full">
                <label className="label is-small">Cycle Progress</label>
                <progress className="progress is-info is-small" value={espOutputs.cycleProgress || 0} max="100">
                  {espOutputs.cycleProgress || 0}%
                </progress>
              </div>
              <div className="column is-three-quarters">
                <label className="label is-small"># Cycles Completed</label>
                <input
                    type="text"
                    className="input is-small"
                    value={
                        `${espOutputs.cyclesCompleted || 0} of ${parameters.numberOfCycles || 0} cycles completed`
                    }
                    readOnly
                />
              </div>
              <div className="column is-full">
                <label className="label is-small">Temperature Data</label>
                <input
                  type="text"
                  value={
                    (typeof lastTempRef.current === 'number' && !isNaN(lastTempRef.current))
                      ? `${lastTempRef.current.toFixed(2)} °C`
                      : 'N/A'
                  }
                  readOnly
                />
              </div>
              <div className="column is-full">
                <label className="label is-small">% of Syringe Left</label>
                <progress className="progress is-primary is-small" value={100 - (espOutputs.syringeUsed || 0)} max="100">
                  {(100 - (espOutputs.syringeUsed || 0)).toFixed(2)}% (ESP: {espOutputs.syringeUsed || 0})
                </progress>
              </div>
              <div className="column is-full">
                <label className="label is-small">Extraction Ready</label>
                <span
                  className="tag is-medium"
                  style={{
                    backgroundColor: espOutputs.extractionReady === 'ready' ? 'green' : 'red',
                    color: 'white',
                  }}
                >
                  {espOutputs.extractionReady === 'ready' ? 'Extraction Ready' : 'Not Ready'}
                </span>
              </div>


            </div>
          </div>
        </div>
      </div>

      {/* Recovery State (Debug) at the bottom */}
      <div style={{ position: 'relative', marginTop: '2rem' }}>
        <section className="box mt-4">
          <div className="is-flex is-align-items-center mb-2">
            <h2 className="title is-5 mb-0 mr-3" style={{ marginBottom: 0 }}>Recovery State (Debug)</h2>
            <button className="button is-small mr-2" onClick={() => sendRecoveryUpdate({ cycleStatus: 'paused' })}>
              Send Recovery Update
            </button>
            <button className="button is-small is-danger" onClick={resetRecoveryData}>
              Reset Recovery Data & Restart ESP32
            </button>
          </div>
          <pre>{JSON.stringify(recoveryState, null, 2)}</pre>
        </section>
      </div>
    </div>
  );
}

export default App;
