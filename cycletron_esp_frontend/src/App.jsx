import './App.css';
import React, { useEffect, useRef, useState } from 'react';
import Chart from 'chart.js/auto';
import 'bulma/css/bulma.min.css';


function App() {
  const [isCycleActive, setIsCycleActive] = useState(false); // Track if a cycle is active

  const [currentTemp, setCurrentTemp] = useState(null); // Text Box for current temperature (next to chart)

  // For ESP outputs and progress bars
  const [syringeLimit, setSyringeLimit] = useState(0); // Syringe limit as a percentage
  const [movementLimits, setMovementLimits] = useState('N/A'); // Movement limits as text
  const [cyclesCompleted, setCyclesCompleted] = useState(0); // Number of cycles completed
  const [cycleProgress, setCycleProgress] = useState(0); // Cycle progress as a percentage
  const [syringeUsed, setSyringeUsed] = useState(0); // Syringe usage as a percentage

  const [espOnline, setEspOnline] = useState(false);
  const [lastMessageTime, setLastMessageTime] = useState(Date.now());
  const [socket, setSocket] = useState(null);
  const chartRef = useRef(null);
  const chartInstanceRef = useRef(null);
  const chartData = useRef([]);
  const tempQueue = useRef([]); // Queue to store temperature data until the chart is initialized
  const [parameters, setParameters] = useState({
    volumeAddedPerCycle: '',
    durationOfRehydration: '',
    syringeDiameter: '',
    desiredHeatingTemperature: '',
    durationOfHeating: '',
    sampleZonesToMix: [], // Initialize as an empty array
    durationOfMixing: '',
    numberOfCycles: ''
  });

  const [activeTab, setActiveTab] = useState('parameters'); // Default to the "parameters" tab
  const [isChartInitialized, setIsChartInitialized] = useState(false);

  const [buttonStates, setButtonStates] = useState({
    extract: false,
    refill: false,
    startCycle: false,
    pauseCycle: false,
    endCycle: false,
    logCycle: false,
  });

  useEffect(() => {
    let ws;
    let reconnectTimeout;

    const connectWebSocket = () => {
      ws = new WebSocket('ws://169.233.112.228/ws');

      ws.onopen = () => {
        console.log('WebSocket connected');
        setEspOnline(true);
        setSocket(ws);
      };
      // const connectWebSocket = () => {
      //   const esp32Ip = process.env.REACT_APP_ESP32_IP;
      //   const esp32Port = process.env.REACT_APP_ESP32_PORT;
      //   const wsUrl = `ws://${esp32Ip}:${esp32Port}/ws`;

      //   const ws = new WebSocket(wsUrl);

      //   ws.onopen = () => {
      //     console.log('WebSocket connected to:', wsUrl);
      //     setEspOnline(true);
      //     setSocket(ws);
      //   };

      //   ws.onclose = () => {
      //     console.log('WebSocket disconnected');
      //     setEspOnline(false);
      //   };

      //   ws.onerror = (err) => {
      //     console.error('WebSocket error:', err);
      //     setEspOnline(false);
      //   };
      // };

      ws.onmessage = (event) => {
        try {
          const msg = JSON.parse(event.data); // Parse the WebSocket message
          console.log("WebSocket message received:", msg); // Debug log

          setLastMessageTime(Date.now());

          if (msg.type === 'temperature') {
            const value = msg.value;

            console.log("Temperature received:", value);
            setCurrentTemp(value); // Update the currentTemp state

            if (isChartInitialized && chartInstanceRef.current) {
              // Update the chart if it is initialized
              chartData.current.push(value);

              if (chartData.current.length > 20) chartData.current.shift();

              const labels = chartData.current.map((_, i) => i + 1);
              chartInstanceRef.current.data.labels = labels;
              chartInstanceRef.current.data.datasets[0].data = [...chartData.current];
              chartInstanceRef.current.update();
            } else {
              // Queue the data if the chart is not initialized
              console.warn("Chart instance is not initialized yet. Queuing data.");
              tempQueue.current.push(value);
            }
          }

          if (msg.name && msg.state) {
            setGpioStates((prev) => ({ ...prev, [msg.name]: msg.state }));
          }
        } catch (err) {
          console.error("Malformed WebSocket message:", event.data); // Log the raw message
          console.error("Error details:", err); // Log the error details
        }
      };

      ws.onclose = () => {
        console.log('WebSocket disconnected');
        setEspOnline(false);
        reconnect(); // ⏳ Try to reconnect
      };

      ws.onerror = (err) => {
        console.error('WebSocket error:', err);
        setEspOnline(false);
        ws.close(); // Force close, triggers onclose
      };
    };

    const reconnect = () => {
      reconnectTimeout = setTimeout(() => {
        console.log('Attempting to reconnect...');
        connectWebSocket();
      }, 3); // Retry after 3 seconds
    };

    connectWebSocket(); // First connection attempt

    return () => {
      if (ws) ws.close();
      clearTimeout(reconnectTimeout);
    };
  }, []);



  const sendGPIOCommand = (name, state) => {
    if (socket && socket.readyState === WebSocket.OPEN && gpioStates[name] !== state) {
      socket.send(JSON.stringify({ name, state }));
      // Optimistically update state before ESP32 confirms
      setGpioStates(prev => ({ ...prev, [name]: state }));
    }
  };

  const sendParameters = () => {
    if (socket && socket.readyState === WebSocket.OPEN) {
      socket.send(JSON.stringify({ type: 'parameters', data: parameters }));
      console.log('Parameters sent:', parameters);
    } else {
      console.error('WebSocket is not connected.');
    }
  };

  const handleGoButton = () => {
    sendParameters(); // Send parameters to the backend
    setIsCycleActive(true); // Lock the "Set Parameters" tab
    setActiveTab('controls'); // Switch to the "Controls" tab
  };

  const handleEndCycleButton = () => {
    setIsCycleActive(false); // Unlock the "Set Parameters" tab
    console.log('Cycle ended.');

    // Temporarily turn the button green
    setButtonStates((prev) => ({ ...prev, endCycle: true }));
    setTimeout(() => {
      setButtonStates((prev) => ({ ...prev, endCycle: false })); // Reset the button state after 3 seconds
    }, 1000);
  };


  const sendButtonCommand = (buttonName) => {
    if (socket && socket.readyState === WebSocket.OPEN) {
      const newState = !buttonStates[buttonName];
      setButtonStates((prev) => ({ ...prev, [buttonName]: newState }));

      // Send the button state to the ESP32
      socket.send(JSON.stringify({ type: 'button', name: buttonName, state: newState ? 'on' : 'off' }));
      console.log(`Button ${buttonName} sent with state: ${newState ? 'on' : 'off'}`);
    } else {
      console.error('WebSocket is not connected.');
    }
  };

  useEffect(() => {
    const interval = setInterval(() => {
      const now = Date.now();
      const secondsSinceLastMsg = (now - lastMessageTime) / 1000;

      if (secondsSinceLastMsg > 6) {
        setEspOnline(false);
      }
    }, 3000);

    return () => clearInterval(interval);
  }, [lastMessageTime]);


  useEffect(() => {
    const ctx = chartRef.current?.getContext('2d');
    if (!ctx) {
      console.error("Chart context not found");
      return;
    }

    // Destroy any existing chart instance to avoid conflicts
    if (chartInstanceRef.current) {
      chartInstanceRef.current.destroy();
    }

    // Initialize the chart
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
          y: {
            beginAtZero: true,
            suggestedMax: 100
          }
        }
      }
    });

    setIsChartInitialized(true);

    // Process any queued data immediately
    if (tempQueue.current.length > 0) {
      console.log("Processing queued data...");
      chartData.current = [...tempQueue.current];
      tempQueue.current = [];
      updateChart();
    }
  }, []);

  const updateChart = () => {
    if (!chartInstanceRef.current) {
      console.error("Chart instance is not initialized.");
      return;
    }

    const labels = chartData.current.map((_, i) => i + 1);
    chartInstanceRef.current.data.labels = labels;
    chartInstanceRef.current.data.datasets[0].data = [...chartData.current];
    chartInstanceRef.current.update();
  };

  useEffect(() => {
    if (currentTemp !== null && isChartInitialized) {
      chartData.current.push(currentTemp);
      if (chartData.current.length > 20) chartData.current.shift();
      updateChart();
    }
  }, [currentTemp, isChartInitialized]);

  // WebSocket message handling
  useEffect(() => {
    const connectWebSocket = () => {
      const ws = new WebSocket('ws://169.233.112.228/ws');

      ws.onopen = () => {
        console.log('WebSocket connected');
        setEspOnline(true);
        setSocket(ws);
      };

      ws.onmessage = (event) => {
        try {
          const msg = JSON.parse(event.data);
          console.log("WebSocket message received:", msg);

          setLastMessageTime(Date.now());

          if (msg.type === 'temperature') {
            setCurrentTemp(msg.value);
          }

          if (msg.type === 'status') {
            if (msg.syringeLimit !== undefined) setSyringeLimit(msg.syringeLimit);
            if (msg.movementLimits !== undefined) setMovementLimits(msg.movementLimits);
            if (msg.cyclesCompleted !== undefined) setCyclesCompleted(msg.cyclesCompleted);
            if (msg.cycleProgress !== undefined) setCycleProgress(msg.cycleProgress);
            if (msg.syringeUsed !== undefined) setSyringeUsed(msg.syringeUsed);
          }
        } catch (err) {
          console.error("Malformed WebSocket message:", event.data);
          console.error("Error details:", err);
        }
      };

      ws.onclose = () => {
        console.log('WebSocket disconnected');
        setEspOnline(false);
        setTimeout(connectWebSocket, 3000); // Reconnect after 3 seconds
      };

      ws.onerror = (err) => {
        console.error('WebSocket error:', err);
        setEspOnline(false);
        ws.close();
      };
    };

    connectWebSocket();

    return () => {
      if (socket) socket.close();
    };
  }, [isChartInitialized]);

  return (
    // <div className="container mt-5">
    <div className="container">
      <h1 className="title is-2" style={{ marginTop: '0' }}>Wet-Dry Cycler Interface</h1>

      <div className="mb-4">
        <span className="tag is-medium" style={{ backgroundColor: espOnline ? "green" : "red" }}></span>
        <span className="ml-2">
          ESP32 Status: <strong>{espOnline ? "Connected" : "Disconnected"}</strong>
        </span>
      </div>

      <div className="columns">
        {/* Tabs Section */}
        <div className="column is-three-quarters">
          <div className="tabs is-toggle is-fullwidth" style={{ maxWidth: '600px', margin: '0' }}>
            <ul>
              <li className={activeTab === 'parameters' ? 'is-active' : ''}>
                <a
                  onClick={() => {
                    if (!isCycleActive) setActiveTab('parameters');
                  }}
                  style={{ pointerEvents: isCycleActive ? 'none' : 'auto', opacity: isCycleActive ? 0.5 : 1 }}
                >
                  Set Parameters
                </a>
              </li>
              <li className={activeTab === 'controls' ? 'is-active' : ''}>
                <a onClick={() => setActiveTab('controls')}>Controls</a>
              </li>
            </ul>
          </div>

          <div className="box" style={{ maxWidth: '600px', margin: '0' }}>
            {/* Existing Tab Content */}
            {activeTab === 'parameters' && (
              <section className="mt-4">
                {/* <h2 className="title is-4">Set Parameters</h2> */}
                <div className="columns is-multiline">
                  {[
                    { label: "Volume Added Per Cycle (mL)", key: "volumeAddedPerCycle", placeholder: "e.g., 10" },
                    { label: "Duration of Rehydration (seconds)", key: "durationOfRehydration", placeholder: "e.g., 30" },
                    { label: "Syringe Diameter (mm)", key: "syringeDiameter", placeholder: "e.g., 5" },
                    { label: "Desired Heating Temperature (°C)", key: "desiredHeatingTemperature", placeholder: "e.g., 90" },
                    { label: "Duration of Heating (seconds)", key: "durationOfHeating", placeholder: "e.g., 120" },
                    { label: "Duration of Mixing (seconds)", key: "durationOfMixing", placeholder: "e.g., 15" },
                    { label: "Number of Cycles", key: "numberOfCycles", placeholder: "e.g., 5" },
                  ].map(({ label, key, placeholder }) => (
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
                              if (value === '' || Number(value) > 0) setParameters({ ...parameters, [key]: value });
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
                                setParameters({ ...parameters, sampleZonesToMix: updatedZones });
                              }}
                            />
                            {zone}
                          </label>
                        ))}
                      </div>
                    </div>
                  </div>
                </div>
                {/* Add the Go button */}
                <div className="field">
                  <div className="control">
                    <button
                      className="button is-primary"
                      onClick={handleGoButton}
                    >
                      Go
                    </button>
                  </div>
                </div>
              </section>
            )}

            {activeTab === 'controls' && (
              <section className="mt-4">
                {/* <h2 className="title is-4">Controls</h2> */}
                <div className="mb-4">
                  {/* <h3 className="title is-5">Live Temperature (°C)</h3>
                  <input
                    type="text"
                    className="input is-small"
                    style={{ width: '100px' }}
                    value={currentTemp !== null ? `${currentTemp} °C` : "N/A"}
                    readOnly
                  />
                  <p className="mt-2 is-size-7">
                    {espOnline ? "Live temperature data updating..." : "ESP32 offline – showing last known data."}
                  </p> */}
                </div>
                <div className="buttons are-medium is-flex is-flex-wrap-wrap is-justify-content-space-between">
                  {[
                    { id: 'startCycle', label: 'Start Cycle' },
                    { id: 'pauseCycle', label: buttonStates['pauseCycle'] ? 'Resume Cycle' : 'Pause Cycle' },
                    { id: 'endCycle', label: 'End Cycle' },
                    { id: 'extract', label: 'Extract (Pause/Continue)' },
                    { id: 'refill', label: 'Refill Syringe (Pause/Continue)' },
                    { id: 'logCycle', label: 'Log Cycle' },
                  ].map(({ id, label }) => (
                    <button
                      key={id}
                      className={`button ${buttonStates[id] ? 'is-success' : 'is-light'} m-2`}
                      onClick={() => {
                        if (id === 'endCycle') handleEndCycleButton(); // Unlock "Set Parameters" on "End Cycle"
                        const newState = !buttonStates[id];
                        setButtonStates((prev) => ({ ...prev, [id]: newState }));

                        // Send the button state to the ESP32
                        if (socket && socket.readyState === WebSocket.OPEN) {
                          socket.send(JSON.stringify({ type: 'button', name: id, state: newState ? 'on' : 'off' }));
                          console.log(`Button ${id} sent with state: ${newState ? 'on' : 'off'}`);
                        } else {
                          console.error('WebSocket is not connected.');
                        }
                      }}
                      disabled={
                        (id !== 'extract' && id !== 'refill' && (buttonStates['extract'] || buttonStates['refill'])) || // Disable other buttons if Extract or Refill is active
                        (id === 'extract' && buttonStates['refill']) || // Disable Extract if Refill is active
                        (id === 'refill' && buttonStates['extract'])    // Disable Refill if Extract is active
                      }
                    >
                      {label}
                    </button>
                  ))}
                </div>
              </section>
            )}
          </div>
        </div>

        {/* Outputs Section */}
        <div className="column is-one-quarter">
          <div className="box" style={{ maxWidth: '200px', margin: '0' }}>
            <h2 className="title is-5">ESP32 Outputs</h2>
            <div className="columns is-full is-multiline">
              {/* Temperature Data */}
              <div className="column is-full">
                <label className="label is-small">Temperature Data</label>
                <input
                  type="text"
                  className="input is-small"
                  value={currentTemp !== null ? `${currentTemp} °C` : "N/A"}
                  readOnly
                />
              </div>

              {/* Syringe Limit */}
              <div className="column is-full">
                <label className="label is-small">Syringe Limit</label>
                <progress
                  className="progress is-primary is-small"
                  value={syringeLimit}
                  max="100"
                >
                  {syringeLimit}%
                </progress>
              </div>

              {/* Movement Limits */}
              <div className="column is-full">
                <label className="label is-small">Movement Limits</label>
                <input
                  type="text"
                  className="input is-small"
                  value={movementLimits}
                  readOnly
                />
              </div>

              {/* # Cycles Completed */}
              <div className="column is-half">
                <label className="label is-small"># Cycles Completed</label>
                <input
                  type="text"
                  className="input is-small"
                  value={cyclesCompleted}
                  readOnly
                />
              </div>

              {/* Cycle Progress */}
              <div className="column is-half">
                <label className="label is-small">Cycle Progress</label>
                <progress
                  className="progress is-info is-small"
                  value={cycleProgress}
                  max="100"
                >
                  {cycleProgress}%
                </progress>
              </div>

              {/* % Syringe Used */}
              <div className="column is-full">
                <label className="label is-small">% Syringe Used</label>
                <progress
                  className="progress is-danger is-small"
                  value={syringeUsed}
                  max="100"
                >
                  {syringeUsed}%
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
