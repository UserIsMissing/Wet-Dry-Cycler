import WebSocket from 'ws';

const ws = new WebSocket('ws://localhost:5000');

ws.on('open', () => {
  console.log('Connected to server, sending fake temperature...');
  ws.send(JSON.stringify({ type: 'temperature', value: 42.5 }));
  ws.close();
});

ws.on('error', console.error);
