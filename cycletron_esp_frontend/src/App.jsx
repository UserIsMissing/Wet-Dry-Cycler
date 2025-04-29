import React, { useState, useEffect } from 'react';

function App() {
  const [socket, setSocket] = useState(null);
  const [ledState, setLedState] = useState(false); // false = OFF

  useEffect(() => {
    const ws = new WebSocket('ws://10.0.0.229/ws'); // 🔥 IMPORTANT: Replace 192.168.1.45 with your ESP32 IP
    ws.onopen = () => {
      console.log('WebSocket connected');
    };
    ws.onerror = (error) => {
      console.error('WebSocket error:', error);
    };
    ws.onclose = () => {
      console.log('WebSocket disconnected');
    };
    setSocket(ws);

    return () => {
      ws.close();
    };
  }, []);

  const turnLedOn = () => {
    if (socket && socket.readyState === WebSocket.OPEN) {
      socket.send(JSON.stringify({ state: true }));
      setLedState(true);
    }
  };

  const turnLedOff = () => {
    if (socket && socket.readyState === WebSocket.OPEN) {
      socket.send(JSON.stringify({ state: false }));
      setLedState(false);
    }
  };

  return (
    <div style={{ textAlign: 'center', marginTop: '50px' }}>
      <h1>ESP32 LED WebSocket Control</h1>
      <button
        onClick={ledState ? turnLedOff : turnLedOn}
        style={{ fontSize: '20px', padding: '10px 20px', marginTop: '20px' }}
      >
        {ledState ? 'Turn LED OFF' : 'Turn LED ON'}
      </button>
    </div>
  );
}

export default App;
