import { useEffect, useRef, useState, useCallback } from 'react';

const RECONNECT_DELAY = 3000;
const PORT = 5175;

export default function useWebSocket() {
    const socketRef = useRef(null);
    const [espOnline, setEspOnline] = useState(false);
    const [lastMessageTime, setLastMessageTime] = useState(Date.now());
    const [isConnected, setIsConnected] = useState(false);

    const [recoveryState, setRecoveryState] = useState(null);
    const [currentTemp, setCurrentTemp] = useState(null);
    const [espOutputs, setEspOutputs] = useState({
        syringeLimit: 0,
        extractionReady: 'N/A',
        cyclesCompleted: 0,
        cycleProgress: 0,
        syringeUsed: 0,
    });

    const connectWebSocket = useCallback(() => {
        if (socketRef.current) return; // Prevent duplicate connections

        const ws = new WebSocket(`ws://${window.location.hostname}:${PORT}`);
        socketRef.current = ws;

        ws.onopen = () => {
            console.log('WebSocket connected');
            setIsConnected(true);
            setEspOnline(true);
        };

        ws.onmessage = (event) => {
            setLastMessageTime(Date.now());
            try {
                const msg = JSON.parse(event.data);

                // Heartbeat from ESP32
                if (msg.type === 'heartbeat' && msg.from === 'esp32') {
                    console.log('Heartbeat received from ESP32');
                    setEspOnline(true);
                    return;
                }

                switch (msg.type) {
                    case 'recoveryState':
                        setRecoveryState(msg.data);
                        break;
                    case 'temperature':
                        setCurrentTemp(msg.value);
                        break;
                    case 'status':
                        setEspOutputs((prev) => ({
                            ...prev,
                            ...(msg.syringeLimit !== undefined && { syringeLimit: msg.syringeLimit }),
                            ...(msg.extractionReady !== undefined && { extractionReady: msg.extractionReady }),
                            ...(msg.cyclesCompleted !== undefined && { cyclesCompleted: msg.cyclesCompleted }),
                            ...(msg.cycleProgress !== undefined && { cycleProgress: msg.cycleProgress }),
                            ...(msg.syringeUsed !== undefined && { syringeUsed: msg.syringeUsed }),
                        }));
                        break;
                    default:
                        // Unknown message type
                        break;
                }
            } catch (err) {
                console.error("Malformed WebSocket message:", event.data, err);
            }
        };

        ws.onclose = () => {
            console.log('WebSocket disconnected');
            setEspOnline(false);
            setIsConnected(false);
            socketRef.current = null;
            setTimeout(connectWebSocket, RECONNECT_DELAY);
        };

        ws.onerror = (err) => {
            console.error('WebSocket error:', err);
            ws.close();
        };
    }, []);

    // Connect on mount
    useEffect(() => {
        connectWebSocket();
        return () => {
            if (socketRef.current) {
                socketRef.current.close();
                socketRef.current = null;
                setIsConnected(false);
            }
        };
    }, [connectWebSocket]);

    // Watchdog to check ESP silence
    useEffect(() => {
        const interval = setInterval(() => {
            const secondsSinceLastMsg = (Date.now() - lastMessageTime) / 1000;
            setEspOnline(secondsSinceLastMsg < 6);
        }, 3000);
        return () => clearInterval(interval);
    }, [lastMessageTime]);

    const sendMessage = useCallback((obj) => { 
        if (socketRef.current?.readyState === WebSocket.OPEN) {
            console.log("Sending message:", obj);
            socketRef.current.send(JSON.stringify(obj));
            return true;
        }
        console.warn('WebSocket is not connected. ReadyState:', socketRef.current?.readyState);
        return false;
    }, []);

    const sendParameters = useCallback(
        (parameters) => sendMessage({ type: 'parameters', data: parameters }),
        [sendMessage]
    );

    const sendButtonCommand = useCallback(
        (name, state) => {
            const payload = { type: 'button', name, state: state ? 'on' : 'off' };
            console.log("sendButtonCommand called with:", payload);
            if (!sendMessage(payload)) {
                console.warn('Failed to send button command: WebSocket not connected');
            }
        },
        [sendMessage]
    );

    const sendRecoveryUpdate = useCallback(
        (data) => sendMessage({ type: 'updateRecoveryState', data }),
        [sendMessage]
    );

    return {
        espOnline,
        recoveryState,
        currentTemp,
        espOutputs,
        sendParameters,
        sendButtonCommand,
        sendRecoveryUpdate,
        isConnected, // expose if you want to disable UI buttons when disconnected
    };
}
