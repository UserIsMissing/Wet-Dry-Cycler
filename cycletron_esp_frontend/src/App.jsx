import './App.css';
import React, { useEffect, useRef, useState, useCallback } from 'react';
import Chart from 'chart.js/auto';
import 'bulma/css/bulma.min.css';
import useWebSocket from './hooks/useWebSocket';

// Constants
const TAB_WIDTH = 690;
const ESP32_IP = '10.0.0.229';
const MAX_CHART_DATA_POINTS = 20;
const RECONNECT_DELAY = 3000;

// Initial states
const INITIAL_PARAMETERS = {
  volumeAddedPerCycle: '',
  durationOfRehydration: '',
  syringeDiameter: '',
  desiredHeatingTemperature: '',
  durationOfHeating: '',
  sampleZonesToMix: [],
  durationOfMixing: '',
  numberOfCycles: ''
};

const INITIAL_BUTTON_STATES = {
  extract: false,
  refill: false,
  startCycle: false,
  pauseCycle: false,
  endCycle: false,
  logCycle: false,
};

function App() {
  // State hooks
  const [recoveryState, setRecoveryState] = useState(null);
  const [isCycleActive, setIsCycleActive] = useState(false);
  const [currentTemp, setCurrentTemp] = useState(null);
  const [espOnline, setEspOnline] = useState(false);
  const [lastMessageTime, setLastMessageTime] = useState(Date.now());
  const [activeTab, setActiveTab] = useState('parameters');
  const [isChartInitialized, setIsChartInitialized] = useState(false);
  
  // ESP32 outputs state
  const [espOutputs, setEspOutputs] = useState({
    syringeLimit: 0,
    extractioReady: 'N/A',
    cyclesCompleted: 0,
    cycleProgress: 0,
    syringeUsed: 0
  });
  
  const [parameters, setParameters] = useState(INITIAL_PARAMETERS);
  const [buttonStates, setButtonStates] = useState(INITIAL_BUTTON_STATES);
  
  // Refs
  const socketRef = useRef(null);
  const chartRef = useRef(null);
  const chartInstanceRef = useRef(null);
  const chartDataRef = useRef([]);
  
  // WebSocket hook
  const { sendRecoveryUpdate } = useWebSocket(setRecoveryState);

  // WebSocket connection management
  const connectWebSocket = useCallback(() => {
    const ws = new WebSocket(`ws://${ESP32_IP}/ws`);
    
    ws.onopen = () => {
      console.log('WebSocket connected');
      setEspOnline(true);
      socketRef.current = ws;
    };

    ws.onmessage = (event) => {
      try {
        const msg = JSON.parse(event.data);
        console.log("WebSocket message received:", msg);
        setLastMessageTime(Date.now());

        // Handle different message types
        switch (msg.type) {
          case 'recoveryState':
            setRecoveryState(msg.data);
            break;
          case 'temperature':
            setCurrentTemp(msg.value);
            break;
          case 'status':
            setEspOutputs(prev => ({
              ...prev,
              ...(msg.syringeLimit !== undefined && { syringeLimit: msg.syringeLimit }),
              ...(msg.extractioReady !== undefined && { extractioReady: msg.extractioReady }),
              ...(msg.cyclesCompleted !== undefined && { cyclesCompleted: msg.cyclesCompleted }),
              ...(msg.cycleProgress !== undefined && { cycleProgress: msg.cycleProgress }),
              ...(msg.syringeUsed !== undefined && { syringeUsed: msg.syringeUsed })
            }));
            break;
          default:
            if (msg.name && msg.state !== undefined) {
              // Handle GPIO state updates if needed
            }
        }
      } catch (err) {
        console.error("Malformed WebSocket message:", event.data, err);
      }
    };

    ws.onclose = () => {
      console.log('WebSocket disconnected');
      setEspOnline(false);
      setTimeout(connectWebSocket, RECONNECT_DELAY);
    };

    ws.onerror = (err) => {
      console.error('WebSocket error:', err);
      setEspOnline(false);
      ws.close();
    };

    return ws;
  }, []);

  // Initialize WebSocket connection
  useEffect(() => {
    const ws = connectWebSocket();
    return () => {
      if (ws) ws.close();
    };
  }, [connectWebSocket]);

  // ESP32 online status check
  useEffect(() => {
    const interval = setInterval(() => {
      const secondsSinceLastMsg = (Date.now() - lastMessageTime) / 1000;
      if (secondsSinceLastMsg > 6) setEspOnline(false);
    }, 3000);

    return () => clearInterval(interval);
  }, [lastMessageTime]);

  // Chart initialization
  useEffect(() => {
    const ctx = chartRef.current?.getContext('2d');
    if (!ctx) return;

    if (chartInstanceRef.current) {
      chartInstanceRef.current.destroy();
    }

    chartInstanceRef.current = new Chart(ctx, {
      type: 'line',
      data: {
        labels: [],
        datasets: [{
          label: 'Temperature (°C)',
          data: [],
          borderColor: 'orange',
          backgroundColor: 'moccasin',
          fill: false,
          tension: 0.3
        }]
      },
      options: {
        animation: false,
        scales: {
          y: { beginAtZero: true, suggestedMax: 100 }
        }
      }
    });

    setIsChartInitialized(true);
    return () => {
      if (chartInstanceRef.current) {
        chartInstanceRef.current.destroy();
      }
    };
  }, []);

  // Chart update
  const updateChart = useCallback(() => {
    if (!chartInstanceRef.current) return;

    const labels = chartDataRef.current.map((_, i) => i + 1);
    chartInstanceRef.current.data.labels = labels;
    chartInstanceRef.current.data.datasets[0].data = [...chartDataRef.current];
    chartInstanceRef.current.update();
  }, []);

  useEffect(() => {
    if (currentTemp !== null && isChartInitialized) {
      chartDataRef.current.push(currentTemp);
      if (chartDataRef.current.length > MAX_CHART_DATA_POINTS) {
        chartDataRef.current.shift();
      }
      updateChart();
    }
  }, [currentTemp, isChartInitialized, updateChart]);

  // Event handlers
  const handleRecoveryUpdate = () => {
    const recoveryData = {
      cycleStatus: 'paused',
      step: 'refill',
      timestamp: Date.now()
    };
    sendRecoveryUpdate(recoveryData);
    console.log('Sent recovery update:', recoveryData);
  };

  const sendParameters = useCallback(() => {
    if (socketRef.current?.readyState === WebSocket.OPEN) {
      socketRef.current.send(JSON.stringify({ type: 'parameters', data: parameters }));
      console.log('Parameters sent:', parameters);
    } else {
      console.error('WebSocket is not connected.');
    }
  }, [parameters]);

  const handleGoButton = () => {
    sendParameters();
    setIsCycleActive(true);
    setActiveTab('controls');
  };

  const handleEndCycle = useCallback(() => {
    setIsCycleActive(false);
    setActiveTab('parameters');
    setButtonStates(prev => ({ ...prev, startCycle: false }));
    
    // Flash the end cycle button
    setButtonStates(prev => ({ ...prev, endCycle: true }));
    setTimeout(() => {
      setButtonStates(prev => ({ ...prev, endCycle: false }));
    }, 1000);
  }, []);

  const sendButtonCommand = useCallback((buttonName) => {
    if (socketRef.current?.readyState === WebSocket.OPEN) {
      const newState = !buttonStates[buttonName];
      
      setButtonStates(prev => ({
        ...prev,
        [buttonName]: buttonName === 'startCycle' ? true : newState
      }));

      socketRef.current.send(JSON.stringify({
        type: 'button',
        name: buttonName,
        state: newState ? 'on' : 'off'
      }));
      
      console.log(`Button ${buttonName} sent with state: ${newState ? 'on' : 'off'}`);
    } else {
      console.error('WebSocket is not connected.');
    }
  }, [buttonStates]);

  // Button configuration
  const buttonConfigs = [
    { id: 'startCycle', label: 'Start Cycle' },
    { id: 'pauseCycle', label: buttonStates.pauseCycle ? 'Resume Cycle' : 'Pause Cycle' },
    { id: 'endCycle', label: 'End Cycle' },
    { id: 'extract', label: 'Extract (Pause/Continue)' },
    { id: 'refill', label: 'Refill Syringe (Pause/Continue)' },
    { id: 'logCycle', label: 'Log Cycle' },
  ];

  // Parameter fields configuration
  const parameterFields = [
    { label: "Volume Added Per Cycle (uL)", key: "volumeAddedPerCycle", placeholder: "e.g., 10" },
    { label: "Duration of Rehydration (seconds)", key: "durationOfRehydration", placeholder: "e.g., 30" },
    { label: "Syringe Diameter (in)", key: "syringeDiameter", placeholder: "e.g., 5" },
    { label: "Desired Heating Temperature (°C)", key: "desiredHeatingTemperature", placeholder: "e.g., 90" },
    { label: "Duration of Heating (seconds)", key: "durationOfHeating", placeholder: "e.g., 120" },
    { label: "Duration of Mixing (seconds)", key: "durationOfMixing", placeholder: "e.g., 15" },
    { label: "Number of Cycles", key: "numberOfCycles", placeholder: "e.g., 5" },
  ];

  return (
    <div className="container">
      {/* Recovery State Debugging */}
      <section className="box mt-4">
        <h2 className="title is-5">Recovery State (Debug)</h2>
        <pre>{JSON.stringify(recoveryState, null, 2)}</pre>
        <button className="button is-small mt-2" onClick={handleRecoveryUpdate}>
          Send Recovery Update
        </button>
      </section>

      <h1 className="title is-2" style={{ marginTop: '0' }}>Wet-Dry Cycler Interface</h1>

      <div className="mb-4">
        <span className="tag is-medium" style={{ backgroundColor: espOnline ? "green" : "red" }}></span>
        <span className="ml-2">
          ESP32 Status: <strong>{espOnline ? "Connected" : "Disconnected"}</strong>
        </span>
      </div>

      <div className="columns">
        {/* Main Content Column */}
        <div className="column is-three-quarters">
          {/* Tabs */}
          <div className="tabs is-toggle is-fullwidth" style={{ maxWidth: TAB_WIDTH, margin: '0' }}>
            <ul>
              <li className={activeTab === 'parameters' ? 'is-active' : ''}>
                <a
                  onClick={() => !isCycleActive && setActiveTab('parameters')}
                  style={{ pointerEvents: isCycleActive ? 'none' : 'auto', opacity: isCycleActive ? 0.5 : 1 }}
                >
                  Set Parameters
                </a>
              </li>
              <li className={activeTab === 'controls' ? 'is-active' : ''}>
                <a
                  onClick={() => isCycleActive && setActiveTab('controls')}
                  style={{ pointerEvents: isCycleActive ? 'auto' : 'none', opacity: isCycleActive ? 1 : 0.5 }}
                >
                  Controls
                </a>
              </li>
            </ul>
          </div>

          {/* Tab Content */}
          <div className="box" style={{ maxWidth: TAB_WIDTH, margin: '0' }}>
            {activeTab === 'parameters' && (
              <section className="mt-4">
                <div className="columns is-multiline">
                  {parameterFields.map(({ label, key, placeholder }) => (
                    <div className="column is-half" key={key}>
                      <div className="field">
                        <label className="label">{label}</label>
                        <div className="control">
                          <input
                            type="number"
                            className="input is-small"
                            placeholder={placeholder}
                            value={parameters[key]}
                            onChange={(e) => {
                              const value = e.target.value;
                              if (value === '' || Number(value) > 0) {
                                setParameters(prev => ({ ...prev, [key]: value }));
                              }
                            }}
                          />
                        </div>
                      </div>
                    </div>
                  ))}
                  <div className="column is-half">
                    <div className="field">
                      <label className="label">Sample Zones to Mix</label>
                      <div className="control">
                        {["Zone1", "Zone2", "Zone3"].map((zone) => (
                          <label key={zone} className="checkbox mr-3">
                            <input
                              type="checkbox"
                              className="mr-2"
                              checked={parameters.sampleZonesToMix.includes(zone)}
                              onChange={(e) => {
                                const updatedZones = e.target.checked
                                  ? [...parameters.sampleZonesToMix, zone]
                                  : parameters.sampleZonesToMix.filter((z) => z !== zone);
                                setParameters(prev => ({ ...prev, sampleZonesToMix: updatedZones }));
                              }}
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
                  {buttonConfigs.map(({ id, label }) => {
                    const isDisabled = 
                      (id === 'startCycle' && buttonStates.startCycle) ||
                      (id !== 'pauseCycle' && buttonStates.pauseCycle) ||
                      (id !== 'extract' && id !== 'refill' && (buttonStates.extract || buttonStates.refill)) ||
                      (id === 'extract' && buttonStates.refill) ||
                      (id === 'refill' && buttonStates.extract);
                    
                    return (
                      <button
                        key={id}
                        className={`button ${buttonStates[id] ? 'is-success' : 'is-light'} m-2`}
                        onClick={() => {
                          if (id === 'endCycle') handleEndCycle();
                          sendButtonCommand(id);
                        }}
                        disabled={isDisabled}
                      >
                        {label}
                      </button>
                    );
                  })}
                </div>
              </section>
            )}
          </div>
        </div>

        {/* Outputs Column */}
        <div className="column is-one-quarter">
          <div className="box" style={{ maxWidth: '400px', margin: '0' }}>
            <h2 className="title is-5">ESP32 Outputs</h2>
            <div className="columns is-full is-multiline">
              <div className="column is-full">
                <label className="label is-small">Temperature Data</label>
                <input
                  type="text"
                  className="input is-small"
                  value={currentTemp !== null ? `${currentTemp} °C` : "N/A"}
                  readOnly
                />
              </div>

              <div className="column is-full">
                <label className="label is-small">Syringe Limit</label>
                <progress
                  className="progress is-primary is-small"
                  value={espOutputs.syringeLimit}
                  max="100"
                >
                  {espOutputs.syringeLimit}%
                </progress>
              </div>

              <div className="column is-full">
                <label className="label is-small">Extraction Ready</label>
                <div className="field">
                  <span
                    className="tag is-medium"
                    style={{
                      backgroundColor: espOutputs.extractioReady === "ready" ? "green" : "red",
                      color: "white",
                    }}
                  >
                    {espOutputs.extractioReady === "ready" ? "Extraction Ready" : "Not Ready"}
                  </span>
                </div>
              </div>

              <div className="column is-half">
                <label className="label is-small"># Cycles Completed</label>
                <input
                  type="text"
                  className="input is-small"
                  value={espOutputs.cyclesCompleted}
                  readOnly
                />
              </div>

              <div className="column is-half">
                <label className="label is-small">Cycle Progress</label>
                <progress
                  className="progress is-info is-small"
                  value={espOutputs.cycleProgress}
                  max="100"
                >
                  {espOutputs.cycleProgress}%
                </progress>
              </div>

              <div className="column is-full">
                <label className="label is-small">% Syringe Used</label>
                <progress
                  className="progress is-danger is-small"
                  value={espOutputs.syringeUsed}
                  max="100"
                >
                  {espOutputs.syringeUsed}%
                </progress>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

export default App;