import React, { useEffect, useRef, useState } from 'react';
import Chart from 'chart.js/auto';

// Wet-Dry Cycler Main App
function App() {
  const [gpioStates, setGpioStates] = useState({
    led: "off",
    mix1: "off",
    mix2: "off",
    mix3: "off"
  });

  const [espOnline, setEspOnline] = useState(false);
  const chartRef = useRef(null);
  const chartInstanceRef = useRef(null);

  // Set GPIO state via POST request
  const setGPIO = async (name, state) => {
    if (gpioStates[name] === state) return;

    try {
      const res = await fetch('http://localhost:5000/set-led', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ name, state })
      });

      if (!res.ok) throw new Error('ESP32 not reachable');
      const data = await res.json();

      if (!data[name]) throw new Error('Invalid response from server');

      setGpioStates(prev => ({ ...prev, [name]: data[name] }));
      setEspOnline(true);
    } catch (err) {
      console.error(`Failed to set ${name}:`, err);
      setEspOnline(false);
    }
  };

  // Fetch ADC history and update chart
  const updateChart = async () => {
    try {
      const res = await fetch('http://localhost:5000/adc-data');
      if (!res.ok) throw new Error('ESP32 not reachable');
      const data = await res.json();

      const labels = data.history.map((_, i) => i + 1);
      const values = data.history;

      if (chartInstanceRef.current) {
        chartInstanceRef.current.data.labels = labels;
        chartInstanceRef.current.data.datasets[0].data = values;
        chartInstanceRef.current.update();
      }

      setEspOnline(true);
    } catch (err) {
      console.error('Failed to fetch ADC data:', err);
      setEspOnline(false);
    }
  };

  // Chart setup + polling interval
  useEffect(() => {
    const ctx = chartRef.current?.getContext('2d');
    if (!ctx) return;

    if (chartInstanceRef.current) chartInstanceRef.current.destroy();

    chartInstanceRef.current = new Chart(ctx, {
      type: 'line',
      data: {
        labels: [],
        datasets: [{
          label: 'ADC Value',
          data: [],
          borderColor: 'blue',
          backgroundColor: 'lightblue',
          fill: false,
          tension: 0.3
        }]
      },
      options: {
        animation: false,
        scales: {
          y: {
            beginAtZero: true,
            suggestedMax: 4095
          }
        }
      }
    });

    const interval = setInterval(updateChart, 1000);
    return () => clearInterval(interval);
  }, []);

  return (
    <div className="container mt-5">
      <h1 className="title is-3">Wet-Dry Cycler Interface</h1>

      {/* ESP32 status indicator */}
      <div className="mb-4">
        <span
          className="tag is-medium"
          style={{ backgroundColor: espOnline ? "green" : "red" }}
        ></span>
        <span className="ml-2">
          ESP32 Status: <strong>{espOnline ? "Connected" : "Disconnected"}</strong>
        </span>
      </div>

      {/* GPIO controls */}
      <section className="columns is-multiline is-variable is-4">
        {[
          { id: 'led', label: 'LED', onText: 'Turn ON', offText: 'Turn OFF' },
          { id: 'mix1', label: 'Mixing Motor 1', onText: 'Start', offText: 'Stop' },
          { id: 'mix2', label: 'Mixing Motor 2', onText: 'Start', offText: 'Stop' },
          { id: 'mix3', label: 'Mixing Motor 3', onText: 'Start', offText: 'Stop' }
        ].map(device => (
          <div key={device.id} className="column is-half">
            <div className="box p-4">
              <h4 className="subtitle is-5">{device.label}</h4>
              <p className="mb-2">
                Status: <strong>{gpioStates[device.id]?.toUpperCase()}</strong>
              </p>
              <div className="buttons are-small">
                <button
                  className="button is-success"
                  onClick={() => setGPIO(device.id, 'on')}
                >
                  {device.onText}
                </button>
                <button
                  className="button is-danger"
                  onClick={() => setGPIO(device.id, 'off')}
                >
                  {device.offText}
                </button>
              </div>
            </div>
          </div>
        ))}
      </section>

      {/* ADC Graph */}
      <section className="mt-6">
        <h2 className="title is-4">Live ADC Data</h2>
        <canvas ref={chartRef} width="600" height="200"></canvas>
      </section>
    </div>
  );
}

export default App;
