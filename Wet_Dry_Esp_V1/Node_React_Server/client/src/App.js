import React, { useEffect, useRef, useState } from 'react';
import Chart from 'chart.js/auto';

// Main component for the Wet-Dry Cycler UI
function App() {
  const [ledState, setLedState] = useState("off");           // LED state: "on" or "off"
  const [espOnline, setEspOnline] = useState(false);          // ESP32 connection status

  const chartRef = useRef(null);                              // Ref to canvas element
  const chartInstanceRef = useRef(null);                      // Ref to Chart instance

  // Fetch LED state and determine if ESP32 is online
  const fetchLedState = async () => {
    try {
      const res = await fetch('http://localhost:5000/led-state');
      if (!res.ok) throw new Error('ESP32 not reachable');
      const data = await res.json();
      if (!data.led) throw new Error('Invalid LED data');
      setLedState(data.led);
      setEspOnline(true);
    } catch (err) {
      console.error('Failed to fetch LED state:', err);
      setEspOnline(false);
    }
  };

  // Send command to turn LED on or off
  const setLED = async (state) => {
    try {
      const res = await fetch('http://localhost:5000/set-led', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ state })
      });
      if (!res.ok) throw new Error('ESP32 not reachable');
      const data = await res.json();
      if (!data.led) throw new Error('Invalid LED response');
      setLedState(data.led);
      setEspOnline(true);
    } catch (err) {
      console.error('Failed to set LED state:', err);
      setEspOnline(false);
    }
  };

  // Fetch and update ADC data to the chart
  const updateChart = async () => {
    try {
      const response = await fetch('http://localhost:5000/adc-data');
      if (!response.ok) throw new Error('ESP32 not reachable');
      const data = await response.json();
      if (!Array.isArray(data.history)) throw new Error('Invalid ADC data');

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

  // Initialize chart and start polling
  useEffect(() => {
    if (!chartRef.current) return;

    const ctx = chartRef.current.getContext('2d');

    if (chartInstanceRef.current) {
      chartInstanceRef.current.destroy();
    }

    // Create new chart
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

    fetchLedState(); // Initial ESP32 check
    const interval = setInterval(updateChart, 1000); // Poll ADC every second

    return () => clearInterval(interval); // Clean up on unmount
  }, []);

  // Style for ESP32 status indicator light
  const statusDotStyle = {
    display: 'inline-block',
    width: '10px',
    height: '10px',
    borderRadius: '50%',
    backgroundColor: espOnline ? 'green' : 'red',
    marginRight: '8px'
  };

  // UI rendering
  return (
    <div style={{ padding: "20px", fontFamily: "Arial, sans-serif" }}>
      <h1>Wet-Dry Cycler Interface</h1>

      {/* ESP32 Connection Indicator */}
      <p>
        <span style={statusDotStyle}></span>
        ESP32 Status: <strong>{espOnline ? "Connected" : "Disconnected"}</strong>
      </p>

      {/* LED State and Controls */}
      <section>
        <h2>LED Control</h2>
        <p>Status: <strong>{ledState.toUpperCase()}</strong></p>
        <button onClick={() => setLED('on')} style={{ marginRight: "10px" }}>Turn ON</button>
        <button onClick={() => setLED('off')}>Turn OFF</button>
      </section>

      {/* Real-Time ADC Graph */}
      <section style={{ marginTop: "40px" }}>
        <h2>Live ADC Data</h2>
        <canvas ref={chartRef} width="600" height="200"></canvas>
      </section>
    </div>
  );
}

export default App;