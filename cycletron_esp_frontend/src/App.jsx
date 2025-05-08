import React, { useEffect, useRef, useState } from 'react';
import Chart from 'chart.js/auto';
import 'bulma/css/bulma.min.css';


function App() {
  // const [gpioStates, setGpioStates] = useState({
  //   led: "off",
  //   mix1: "off",
  //   mix2: "off",
  //   mix3: "off",
  //   extract: "off"
  // });

  const [currentTemp, setCurrentTemp] = useState(null); // Text Box for current temperature (next to chart)

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
      ws = new WebSocket('ws://10.0.0.229/ws');

      ws.onopen = () => {
        console.log('WebSocket connected');
        setEspOnline(true);
        setSocket(ws);
      };

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
  
    if (chartInstanceRef.current) chartInstanceRef.current.destroy();
  
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
      chartData.current = [...tempQueue.current];
      tempQueue.current = [];
      updateChart();
    }
  }, []);
  
  const updateChart = () => {
    if (!chartInstanceRef.current) return;
  
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

  return (
    <div className="container mt-5">
      <h1 className="title is-3">Wet-Dry Cycler Interface</h1>

      <div className="mb-4">
        <span className="tag is-medium" style={{ backgroundColor: espOnline ? "green" : "red" }}></span>
        <span className="ml-2">
          ESP32 Status: <strong>{espOnline ? "Connected" : "Disconnected"}</strong>
        </span>
      </div>

      <div className="tabs is-toggle is-fullwidth">
        <ul>
          <li className={activeTab === 'parameters' ? 'is-active' : ''}>
            <a onClick={() => setActiveTab('parameters')}>Set Parameters</a>
          </li>
          <li className={activeTab === 'controls' ? 'is-active' : ''}>
            <a onClick={() => setActiveTab('controls')}>Controls</a>
          </li>
        </ul>
      </div>

      {activeTab === 'parameters' && (
        <section className="mt-6">
          <h2 className="title is-4">Set Parameters</h2>
          <table className="table is-bordered is-striped is-hoverable is-fullwidth">
            <thead>
              <tr>
                <th>Parameter</th>
                <th>Value</th>
              </tr>
            </thead>
            <tbody>
              <tr>
                <td>Volume Added Per Cycle (mL)</td>
                <td>
                  <input
                    type="number"
                    className="input is-small"
                    placeholder="e.g., 10"
                    value={parameters.volumeAddedPerCycle}
                    onChange={(e) => {
                      const value = e.target.value;
                      if (value === '' || (Number(value) > 0)) {
                        setParameters({ ...parameters, volumeAddedPerCycle: value });
                      }
                    }}
                  />
                </td>
              </tr>
              <tr>
                <td>Duration of Rehydration (seconds)</td>
                <td>
                  <input
                    type="number"
                    className="input is-small"
                    placeholder="e.g., 30"
                    value={parameters.durationOfRehydration}
                    onChange={(e) => {
                      const value = e.target.value;
                      if (value === '' || (Number(value) > 0)) {
                        setParameters({ ...parameters, durationOfRehydration: value });
                      }
                    }}
                  />
                </td>
              </tr>
              <tr>
                <td>Syringe Diameter (mm)</td>
                <td>
                  <input
                    type="number"
                    className="input is-small"
                    placeholder="e.g., 5"
                    value={parameters.syringeDiameter}
                    onChange={(e) => {
                      const value = e.target.value;
                      if (value === '' || (Number(value) > 0)) {
                        setParameters({ ...parameters, syringeDiameter: value });
                      }
                    }}
                  />
                </td>
              </tr>
              <tr>
                <td>Desired Heating Temperature (°C)</td>
                <td>
                  <input
                    type="number"
                    className="input is-small"
                    placeholder="e.g., 90"
                    value={parameters.desiredHeatingTemperature}
                    onChange={(e) => {
                      const value = e.target.value;
                      if (value === '' || (Number(value) > 0)) {
                        setParameters({ ...parameters, desiredHeatingTemperature: value });
                      }
                    }}
                  />
                </td>
              </tr>
              <tr>
                <td>Duration of Heating (seconds)</td>
                <td>
                  <input
                    type="number"
                    className="input is-small"
                    placeholder="e.g., 120"
                    value={parameters.durationOfHeating}
                    onChange={(e) => {
                      const value = e.target.value;
                      if (value === '' || (Number(value) > 0)) {
                        setParameters({ ...parameters, durationOfHeating: value });
                      }
                    }}
                  />
                </td>
              </tr>
              <tr>
                <td>Sample Zones to Mix</td>
                <td>
                  <div className="field">
                    <label className="checkbox">
                      <input
                        type="checkbox"
                        className="mr-2"
                        checked={parameters.sampleZonesToMix.includes("Zone1")}
                        onChange={(e) => {
                          const updatedZones = e.target.checked
                            ? [...parameters.sampleZonesToMix, "Zone1"]
                            : parameters.sampleZonesToMix.filter(zone => zone !== "Zone1");
                          setParameters({ ...parameters, sampleZonesToMix: updatedZones });
                        }}
                      />
                      Zone 1
                    </label>
                  </div>
                  <div className="field">
                    <label className="checkbox">
                      <input
                        type="checkbox"
                        className="mr-2"
                        checked={parameters.sampleZonesToMix.includes("Zone2")}
                        onChange={(e) => {
                          const updatedZones = e.target.checked
                            ? [...parameters.sampleZonesToMix, "Zone2"]
                            : parameters.sampleZonesToMix.filter(zone => zone !== "Zone2");
                          setParameters({ ...parameters, sampleZonesToMix: updatedZones });
                        }}
                      />
                      Zone 2
                    </label>
                  </div>
                  <div className="field">
                    <label className="checkbox">
                      <input
                        type="checkbox"
                        className="mr-2"
                        checked={parameters.sampleZonesToMix.includes("Zone3")}
                        onChange={(e) => {
                          const updatedZones = e.target.checked
                            ? [...parameters.sampleZonesToMix, "Zone3"]
                            : parameters.sampleZonesToMix.filter(zone => zone !== "Zone3");
                          setParameters({ ...parameters, sampleZonesToMix: updatedZones });
                        }}
                      />
                      Zone 3
                    </label>
                  </div>
                </td>
              </tr>
              <tr>
                <td>Duration of Mixing (seconds)</td>
                <td>
                  <input
                    type="number"
                    className="input is-small"
                    placeholder="e.g., 15"
                    value={parameters.durationOfMixing}
                    onChange={(e) => {
                      const value = e.target.value;
                      if (value === '' || (Number(value) > 0)) {
                        setParameters({ ...parameters, durationOfMixing: value });
                      }
                    }}
                  />
                </td>
              </tr>
              <tr>
                <td>Number of Cycles</td>
                <td>
                  <input
                    type="number"
                    className="input is-small"
                    placeholder="e.g., 5"
                    value={parameters.numberOfCycles}
                    onChange={(e) => {
                      const value = e.target.value;
                      if (value === '' || (Number(value) > 0)) {
                        setParameters({ ...parameters, numberOfCycles: value });
                      }
                    }}
                  />
                </td>
              </tr>
            </tbody>
          </table>
          <button
            className="button is-primary mt-3"
            onClick={() => setActiveTab('controls')} // Switch to the "controls" tab
          >
            Go
          </button>
        </section>
      )}

      {activeTab === 'controls' && (
        <section className="mt-6">
          {/* Temperature Section */}
          <div className="box mb-4">
            <h2 className="title is-4">Live Temperature (°C)</h2>
            <input
              type="text"
              className="input is-small"
              style={{ width: '100px' }}
              value={currentTemp !== null ? `${currentTemp} °C` : "N/A"}
              readOnly
            />
            <p className="mt-2 is-size-7">
              {espOnline ? "Live temperature data updating..." : "ESP32 offline – showing last known data."}
            </p>
          </div>

          {/* Temperature Graph */}
          <div className="box mb-4">
            <canvas ref={chartRef} width="600" 
            height="200"
            style={{ width: "100%", height: "auto" }} // Ensure responsiveness
            ></canvas>
          </div>

          {/* GPIO Controls */}
          {/* <div className="columns is-multiline is-variable is-4">
            {['extract', 'led', 'mix1', 'mix2', 'mix3'].map(id => (
              <div key={id} className="column is-half">
                <div className="box p-4">
                  <h4 className="subtitle is-5 has-text-weight-semibold">{id.toUpperCase()}</h4>
                  <p className="mb-3">
                    Status: <strong>{gpioStates[id]?.toUpperCase()}</strong>
                  </p>
                  <div className="buttons has-addons">
                    <button className="button is-small is-success" onClick={() => sendGPIOCommand(id, 'on')}>On</button>
                    <button className="button is-small is-danger" onClick={() => sendGPIOCommand(id, 'off')}>Off</button>
                  </div>
                </div>
              </div>
            ))}
          </div> */}

          {/* New Buttons */}
          <div className="columns is-multiline is-variable is-4">
            {[
              { id: 'extract', label: 'Extract (Pause/Continue)' },
              { id: 'refill', label: 'Refill Syringe (Pause/Continue)' },
              { id: 'startCycle', label: 'Start Cycle' },
              { id: 'pauseCycle', label: 'Pause Cycle' },
              { id: 'endCycle', label: 'End Cycle' },
              { id: 'logCycle', label: 'Log Cycle' },
            ].map(({ id, label }) => (
              <div key={id} className="column is-half">
                <div className="box p-4">
                  <h4 className="subtitle is-5 has-text-weight-semibold">{label}</h4>
                  <button
                    className={`button is-medium ${buttonStates[id] ? 'is-success' : ''}`}
                    style={{
                      backgroundColor: buttonStates[id] ? 'green' : 'white',
                      color: buttonStates[id] ? 'white' : 'black',
                    }}
                    onClick={() => sendButtonCommand(id)}
                  >
                    {buttonStates[id] ? 'On' : 'Off'}
                  </button>
                </div>
              </div>
            ))}
          </div>
        </section>
      )}
    </div>
  );
}

export default App;
