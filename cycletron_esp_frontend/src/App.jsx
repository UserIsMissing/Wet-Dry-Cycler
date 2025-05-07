import React, { useEffect, useRef, useState } from 'react';
import Chart from 'chart.js/auto';

function App() {
  const [gpioStates, setGpioStates] = useState({
    led: "off",
    mix1: "off",
    mix2: "off",
    mix3: "off"
  });

  const [espOnline, setEspOnline] = useState(false);
  const [socket, setSocket] = useState(null);
  const chartRef = useRef(null);
  const chartInstanceRef = useRef(null);

  useEffect(() => {
    const ws = new WebSocket('ws://169.233.112.251/ws');

    ws.onopen = () => {
      console.log('WebSocket connected');
      setEspOnline(true);
      setSocket(ws);
    };

    ws.onmessage = (event) => {
      try {
        const msg = JSON.parse(event.data);
        if (msg.type === 'temperature') {
          updateChart(msg.history);
        } else if (msg.type === 'gpio') {
          setGpioStates(prev => ({ ...prev, [msg.name]: msg.state }));
        }
      } catch (err) {
        console.error("Malformed WebSocket message:", event.data);
      }
    };

    ws.onclose = () => {
      console.log('WebSocket disconnected');
      setEspOnline(false);
    };

    ws.onerror = (err) => {
      console.error('WebSocket error:', err);
    };

    return () => ws.close();
  }, []);

  const sendGPIOCommand = (name, state) => {
    if (socket && socket.readyState === WebSocket.OPEN && gpioStates[name] !== state) {
      socket.send(JSON.stringify({ type: 'gpio', name, state }));
    }
  };

  const updateChart = async () => {
    try {
      const res = await fetch('http://localhost:5000/api/history');
      const data = await res.json();
      const labels = data.map((_, i) => i + 1);
      const values = data.map(row => row.value);
  
      if (chartInstanceRef.current) {
        chartInstanceRef.current.data.labels = labels;
        chartInstanceRef.current.data.datasets[0].data = values;
        chartInstanceRef.current.update();
      }
  
      setEspOnline(true);
    } catch {
      setEspOnline(false);
    }
  };

  useEffect(() => {
    const ctx = chartRef.current?.getContext('2d');
    if (!ctx) return;

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
  }, []);

  return (
    <div className="container mt-5">
      <h1 className="title is-3">Wet-Dry Cycler Interface</h1>

      <div className="mb-4">
        <span className="tag is-medium" style={{ backgroundColor: espOnline ? "green" : "red" }}></span>
        <span className="ml-2">
          ESP32 Status: <strong>{espOnline ? "Connected" : "Disconnected"}</strong>
        </span>
      </div>

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
                <button className="button is-success" onClick={() => sendGPIOCommand(device.id, 'on')}>
                  {device.onText}
                </button>
                <button className="button is-danger" onClick={() => sendGPIOCommand(device.id, 'off')}>
                  {device.offText}
                </button>
              </div>
            </div>
          </div>
        ))}
      </section>

      <section className="mt-6">
        <h2 className="title is-4">Live Temperature (°C)</h2>
        <canvas ref={chartRef} width="600" height="200"></canvas>
        <p className="mt-2 is-size-7">
          {espOnline ? "Live temperature data updating..." : "ESP32 offline – showing last known data."}
        </p>
      </section>
    </div>
  );
}

export default App;
