import { useEffect, useRef, useState, useCallback } from 'react';

const RECONNECT_DELAY = 3000;
const PORT = 5175;

export default function useWebSocket() {
    const socketRef = useRef(null);
    const [espOnline, setEspOnline] = useState(false);
    const [lastMessageTime, setLastMessageTime] = useState(Date.now());

    // States to expose
    const [recoveryState, setRecoveryState] = useState(null);
    const [currentTemp, setCurrentTemp] = useState(null);
    const [espOutputs, setEspOutputs] = useState({
        syringeLimit: 0,
        extractionReady: 'N/A',
        cyclesCompleted: 0,
        cycleProgress: 0,
        syringeUsed: 0,
    });

    // Connect WebSocket
    const connectWebSocket = useCallback(() => {
        const ws = new WebSocket(`ws://${window.location.hostname}:${PORT}`);

        ws.onopen = () => {
            console.log('WebSocket connected');
            setEspOnline(true);
            socketRef.current = ws;
        };

        ws.onmessage = (event) => {
            setLastMessageTime(Date.now());
            try {
                const msg = JSON.parse(event.data);

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
            setTimeout(connectWebSocket, RECONNECT_DELAY);
        };

        ws.onerror = (err) => {
            console.error('WebSocket error:', err);
            setEspOnline(false);
            ws.close();
        };

        return ws;
    }, []);

    // Initialize WebSocket once on mount
    useEffect(() => {
        const ws = connectWebSocket();
        return () => {
            if (ws) ws.close();
        };
    }, [connectWebSocket]);

    // ESP32 online status check every 3 seconds
    useEffect(() => {
        const interval = setInterval(() => {
            const secondsSinceLastMsg = (Date.now() - lastMessageTime) / 1000;
            if (secondsSinceLastMsg > 6) setEspOnline(false);
        }, 3000);
        return () => clearInterval(interval);
    }, [lastMessageTime]);

    // Send message helper
    const sendMessage = useCallback((obj) => {
        if (socketRef.current?.readyState === WebSocket.OPEN) {
            socketRef.current.send(JSON.stringify(obj));
            return true;
        }
        console.warn('WebSocket is not connected');
        return false;
    }, []);

    // Public API: send parameters
    const sendParameters = useCallback(
        (parameters) => sendMessage({ type: 'parameters', data: parameters }),
        [sendMessage]
    );

    // Public API: send button commands
    const sendButtonCommand = useCallback(
        (name, state) => sendMessage({ type: 'button', name, state: state ? 'on' : 'off' }),
        [sendMessage]
    );

    // Public API: send recovery update
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
    };
}
