import { useEffect, useRef } from 'react';

export default function useWebSocket(onRecoveryStateReceived) {
    const ws = useRef(null);

    useEffect(() => {
        ws.current = new WebSocket(`ws://${window.location.hostname}:5175`);
        // ws.current = new WebSocket('ws://169.233.116.115:5175'); // Ensure this URL matches your server's WebSocket URL
        // ws.current = new WebSocket('ws://localhost:5175'); // Ensure this URL matches your server's WebSocket URL

        // Handle WebSocket connection open event
        ws.current.onopen = () => {
            console.log('WebSocket connected');
            // Ask for recovery state on connect
            ws.current.send(JSON.stringify({ type: 'getRecoveryState' }));
        };

        // Handle WebSocket connection close event
        ws.onclose = (event) => {
            console.log(`WebSocket disconnected. Code: ${event.code}, Reason: ${event.reason}, WasClean: ${event.wasClean}`);
            setEspOnline(false);
        };

        // Handle WebSocket errors
        ws.onerror = (error) => {
            console.error('WebSocket error:', error);
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

        return () => {
            ws.current.close();
        };
    }, [onRecoveryStateReceived]);

    // Shared logic that handles sending and receiving messages between the ESP32 and the frontend
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
