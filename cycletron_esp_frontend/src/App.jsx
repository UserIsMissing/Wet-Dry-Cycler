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
  const chartData = useRef([]); // store a rolling history

  useEffect(() => {
    const ws = new WebSocket('ws://10.0.0.167/ws'); // ← use ESP32 IP here

    ws.onopen = () => {
      console.log('WebSocket connected');
      setEspOnline(true);
      setSocket(ws);
    };

    ws.onmessage = (event) => {
      try {
        const msg = JSON.parse(event.data);

        if (msg.type === 'temperature') {
          const value = msg.value;
          chartData.current.push(value);
          if (chartData.current.length > 20) chartData.current.shift(); // keep chart short

          const labels = chartData.current.map((_, i) => i + 1);
          chartInstanceRef.current.data.labels = labels;
          chartInstanceRef.current.data.datasets[0].data = [...chartData.current];
          chartInstanceRef.current.update();
        }

        if (msg.name && msg.state) {
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
      socket.send(JSON.stringify({ name, state }));
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
        {['led', 'mix1', 'mix2', 'mix3'].map(id => (
          <div key={id} className="column is-half">
            <div className="box p-4">
              <h4 className="subtitle is-5">{id.toUpperCase()}</h4>
              <p className="mb-2">
                Status: <strong>{gpioStates[id]?.toUpperCase()}</strong>
              </p>
              <div className="buttons are-small">
                <button className="button is-success" onClick={() => sendGPIOCommand(id, 'on')}>On</button>
                <button className="button is-danger" onClick={() => sendGPIOCommand(id, 'off')}>Off</button>
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
