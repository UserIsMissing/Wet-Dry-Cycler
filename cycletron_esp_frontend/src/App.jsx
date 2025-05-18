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
  const {
    espOnline,
    recoveryState,
    currentTemp,
    espOutputs,
    sendParameters,
    sendButtonCommand,
    sendRecoveryUpdate,
  } = useWebSocket();

  const [parameters, setParameters] = useState(INITIAL_PARAMETERS);
  const [activeTab, setActiveTab] = useState('parameters');
  const [cycleState, setCycleState] = useState('idle');
  const [activeButton, setActiveButton] = useState(null);
  const [isPaused, setIsPaused] = useState(false);
  const [showVialSetup, setShowVialSetup] = useState(true);
  const [vialSetupStep, setVialSetupStep] = useState('prompt'); // 'prompt', 'continue', or null

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
    sendParameters(parameters);
    sendRecoveryUpdate({
      ...parameters, // add all parameters to recovery state
      machineStep: 'idle',
      lastAction: 'setParameters',
      progress: 0,
    });
    setCycleState('idle');
    setActiveTab('controls');
  };

  const handleStartCycle = () => {
    sendButtonCommand('startCycle', true);
    setCycleState('started');
    setActiveButton(null);
    sendRecoveryUpdate({
      machineStep: 'started', 
      lastAction: 'startCycle',
      progress: 0,
      // add any other state you want to track
    });
  };

  const handlePauseCycle = () => {
    if (activeButton === 'pauseCycle') {
      sendButtonCommand('pauseCycle', false);
      sendButtonCommand('resumeCycle', true);
      setCycleState('started');
      setIsPaused(false);
      setActiveButton(null);
      sendRecoveryUpdate({
        machineStep: 'started',
        lastAction: 'resumeCycle',
        // ...
      });
    } else {
      sendButtonCommand('pauseCycle', true);
      setCycleState('paused');
      setIsPaused(true);
      setActiveButton('pauseCycle');
      sendRecoveryUpdate({
        machineStep: 'paused',
        lastAction: 'pauseCycle',
        // ...
      });
    }
  };

  const handleEndCycle = () => {
    sendButtonCommand('endCycle', true); // send 'on'
    setCycleState('idle');
    setActiveTab('parameters');
    setActiveButton(null);
  };

  const handleExtract = () => {
    const isCanceling = activeButton === 'extract';
    sendButtonCommand('extract', !isCanceling); // send "on" if starting, "off" if canceling
    setCycleState(isCanceling ? 'started' : 'extract');
    setActiveButton(isCanceling ? null : 'extract');
  };

  const handleRefill = () => {
    const isCanceling = activeButton === 'refill';
    sendButtonCommand('refill', !isCanceling); // send "on" if starting, "off" if canceling
    setCycleState(isCanceling ? 'started' : 'refill');
    setActiveButton(isCanceling ? null : 'refill');
  };

  const handleLogCycle = () => {
    sendButtonCommand('logCycle', true, {
      temp: currentTemp,
      timestamp: new Date().toISOString(),
    });
  };

  const isButtonDisabled = (id) => {
    if (id === 'startCycle' && cycleState !== 'idle') return true;
    if (id === 'endCycle' && cycleState === 'idle') return true;
    if (['pauseCycle', 'extract', 'refill'].includes(id) && cycleState === 'idle') return true;
    
    if (activeButton && activeButton !== id) return true;
    if (cycleState === 'idle' && id !== 'startCycle') return true;
    return false;
  };

  return (
    <div className="container" style={{ position: 'relative' }}>
      {/* Vial Setup Overlay */}
      {vialSetupStep && (
        <div
          style={{
            position: 'fixed',
            zIndex: 1000,
            top: 0,
            left: 0,
            width: '100vw',
            height: '100vh',
            background: 'rgba(255,255,255,0.98)',
            display: 'flex',
            flexDirection: 'column',
            alignItems: 'center',
            justifyContent: 'center',
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
                  onClick={() => setVialSetupStep('continue')}
                >
                  Yes
                </button>
                <button
                  className="button is-light is-large"
                  style={{ fontSize: '2rem', padding: '2rem 4rem' }}
                  onClick={() => setVialSetupStep(null)}
                >
                  No
                </button>
              </div>
            </>
          ) : (
            <button
              className="button is-primary is-large"
              style={{ fontSize: '2rem', padding: '2rem 4rem' }}
              onClick={() => setVialSetupStep(null)}
            >
              Continue
            </button>
          )}
        </div>
      )}

      <section className="box mt-4">
        <h2 className="title is-5">Recovery State (Debug)</h2>
        <pre>{JSON.stringify(recoveryState, null, 2)}</pre>
        <button className="button is-small mt-2" onClick={() => sendRecoveryUpdate({ cycleStatus: 'paused' })}>
          Send Recovery Update
        </button>
      </section>

      <h1 className="title is-2">Wet-Dry Cycler Interface</h1>

      <div className="mb-4">
        <span className="tag is-medium" style={{ backgroundColor: espOnline ? 'green' : 'red' }}></span>
        <span className="ml-2">
          ESP32 Status: <strong>{espOnline ? 'Connected' : 'Disconnected'}</strong>
        </span>
      </div>

      <div className="columns">
        <div className="column is-three-quarters">
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
                <label className="label is-small">Temperature Data</label>
                <input
                  type="text"
                  className="input is-small"
                  value={currentTemp !== null ? `${currentTemp} °C` : 'N/A'}
                  readOnly
                />
              </div>
              <div className="column is-full">
                <label className="label is-small">Syringe Limit</label>
                <progress className="progress is-primary is-small" value={espOutputs.syringeLimit} max="100">
                  {espOutputs.syringeLimit}%
                </progress>
              </div>
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
              <div className="column is-half">
                <label className="label is-small"># Cycles Completed</label>
                <input type="text" className="input is-small" value={espOutputs.cyclesCompleted} readOnly />
              </div>
              <div className="column is-half">
                <label className="label is-small">Cycle Progress</label>
                <progress className="progress is-info is-small" value={espOutputs.cycleProgress} max="100">
                  {espOutputs.cycleProgress}%
                </progress>
              </div>
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
