import { useEffect, useRef } from 'react';

export default function useWebSocket(onRecoveryStateReceived) {
    const ws = useRef(null);

    useEffect(() => {
        // ws.current = new WebSocket('ws://localhost:5000');   caused an error "WebSocket connection to 'ws://localhost:5000/' failed:"
        ws.current = new WebSocket('ws://localhost:5175');

        ws.current.onopen = () => {
            console.log('WebSocket connected');
            // Ask for recovery state on connect
            ws.current.send(JSON.stringify({ type: 'getRecoveryState' }));
        };

        ws.current.onmessage = (event) => {
            try {
                const msg = JSON.parse(event.data);
                if (msg.type === 'recoveryState') {
                    console.log('Received recovery state:', msg.data);
                    onRecoveryStateReceived(msg.data);
                }
            } catch (err) {
                console.error('Bad WebSocket message:', err);
            }
        };

        ws.current.onclose = () => {
            console.log('WebSocket disconnected');
        };

        return () => {
            ws.current.close();
        };
    }, [onRecoveryStateReceived]);

    // You can expose a function to send recovery updates
    const sendRecoveryUpdate = (data) => {
        if (ws.current && ws.current.readyState === WebSocket.OPEN) {
            ws.current.send(JSON.stringify({
                type: 'updateRecoveryState',
                data,
            }));
        }
    };

    return { sendRecoveryUpdate };
}
