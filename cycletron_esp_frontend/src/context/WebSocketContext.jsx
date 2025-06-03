import React, { createContext, useContext, useEffect, useRef, useState } from 'react';

const WebSocketContext = createContext();

const RECONNECT_DELAY = 3000;
const PORT = 5175;

export function WebSocketProvider({ children }) {
    const socketRef = useRef(null);
    const reconnectTimeoutRef = useRef(null);
    const isConnectingRef = useRef(false);
    const mountedRef = useRef(true);

    const [espOnline, setEspOnline] = useState(false);
    const [lastEspMessageTime, setLastEspMessageTime] = useState(0); // Start with 0 to force initial detection
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

    const connectWebSocket = () => {
        if (!mountedRef.current) return;
        
        // Prevent multiple simultaneous connection attempts
        if (isConnectingRef.current || 
            socketRef.current?.readyState === WebSocket.CONNECTING || 
            socketRef.current?.readyState === WebSocket.OPEN) {
            console.log('WebSocket connection already in progress or established');
            return;
        }

        isConnectingRef.current = true;
        console.log('Attempting WebSocket connection...');
        
        const ws = new WebSocket(`ws://${window.location.hostname}:${PORT}`);
        socketRef.current = ws;

        ws.onopen = () => {
            if (!mountedRef.current) {
                ws.close();
                return;
            }
            console.log('WebSocket connected successfully');
            isConnectingRef.current = false;
            setIsConnected(true);
            // Don't set espOnline immediately here - wait for actual ESP messages
            ws.send(JSON.stringify({ type: 'getRecoveryState' }));
        };

        ws.onmessage = (event) => {
            if (!mountedRef.current) return;
            
            try {
                const msg = JSON.parse(event.data);

                // Update ESP message timestamp only for ESP32 messages
                // More comprehensive detection of ESP32 messages
                const isEspMessage = msg.from === 'esp32' || 
                    ['heartbeat', 'temperature', 'temperatureUpdate', 'cycleProgress', 
                     'status', 'mixingProgress', 'heatingProgress', 'syringePercentage', 
                     'endOfCycles', 'currentState', 'syringeReset'].includes(msg.type);
                
                if (isEspMessage) {
                    console.log(`🤖 ESP32 message detected: ${msg.type} from: ${msg.from} value: ${msg.value}`);
                    setLastEspMessageTime(Date.now());
                    // Force an immediate check of the ESP status instead of waiting for next watchdog cycle
                    const secondsSinceLastEspMsg = 0; // We just got a message
                    const isEspOnline = true; // Since we just received a message
                    if (!espOnline) {
                        console.log(`ESP32 status changed: Disconnected -> Connected (message received)`);
                        setEspOnline(true);
                    }
                } else {
                    console.log(`💻 Non-ESP32 message: ${msg.type} from: ${msg.from}`);
                }

                // Heartbeat from ESP32
                if (msg.type === 'heartbeat') {
                    console.log('Heartbeat received from ESP32');
                    return;
                }

                switch (msg.type) {
                    case 'recoveryState':
                        setRecoveryState(msg.data);
                        break;
                    case 'temperature':
                        setCurrentTemp(msg.value);
                        break;
                    case 'temperatureUpdate':
                        setCurrentTemp(msg.value);
                        break;
                    case 'cycleProgress':
                        setEspOutputs((prev) => ({
                            ...prev,
                            cyclesCompleted: msg.completed || 0,
                            cycleProgress: msg.percent || 0,
                        }));
                        console.log(`Updated cycle progress: ${msg.completed}/${msg.total} (${msg.percent}%)`);
                        break;
                    case 'syringePercentage':
                        setEspOutputs((prev) => ({
                            ...prev,
                            syringeUsed: msg.value || 0,
                        }));
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
                        break;
                }
            } catch (err) {
                console.error("Malformed WebSocket message:", event.data, err);
            }
        };

        ws.onclose = (event) => {
            console.log('WebSocket disconnected:', event.code, event.reason);
            isConnectingRef.current = false;
            setEspOnline(false);
            setIsConnected(false);
            socketRef.current = null;
            
            // Clear any existing reconnection timeout
            if (reconnectTimeoutRef.current) {
                clearTimeout(reconnectTimeoutRef.current);
                reconnectTimeoutRef.current = null;
            }
            
            // Only reconnect if it wasn't a clean close and component is still mounted
            if (event.code !== 1000 && mountedRef.current) {
                console.log(`Reconnecting in ${RECONNECT_DELAY}ms...`);
                reconnectTimeoutRef.current = setTimeout(connectWebSocket, RECONNECT_DELAY);
            }
        };

        ws.onerror = (err) => {
            console.error('WebSocket error:', err);
            isConnectingRef.current = false;
            setEspOnline(false);
            setIsConnected(false);
        };
    };

    const sendMessage = (obj) => {
        if (socketRef.current?.readyState === WebSocket.OPEN) {
            console.log("Sending message:", obj);
            socketRef.current.send(JSON.stringify(obj));
            return true;
        }
        console.warn('WebSocket is not connected. ReadyState:', socketRef.current?.readyState);
        return false;
    };

    const sendParameters = (parameters) => sendMessage({ type: 'parameters', data: parameters });

    const sendButtonCommand = (name, state, extra = {}) => {
        const payload = { type: 'button', name, state: state ? 'on' : 'off', ...extra };
        console.log("sendButtonCommand called with:", payload);
        if (!sendMessage(payload)) {
            console.warn('Failed to send button command: WebSocket not connected');
        }
    };

    const sendRecoveryUpdate = (data) => sendMessage({ type: 'updateRecoveryState', data });

    const resetRecoveryState = () => {
        fetch('/api/resetRecoveryState', { method: 'POST' })
            .then((res) => res.json())
            .then((data) => {
                if (data.success) {
                    setRecoveryState({});
                    window.location.reload();
                }
            })
            .catch((err) => console.error('Failed to reset recovery state:', err));
    };

    // Initialize connection on mount
    useEffect(() => {
        mountedRef.current = true;
        connectWebSocket();

        // Watchdog to check ESP silence
        const watchdogInterval = setInterval(() => {
            if (mountedRef.current) {
                const secondsSinceLastEspMsg = (Date.now() - lastEspMessageTime) / 1000;
                const isEspOnline = secondsSinceLastEspMsg < 15; // More generous timeout
                
                console.log(`🔍 Watchdog check: ${secondsSinceLastEspMsg.toFixed(1)}s since last ESP32 message, isOnline: ${isEspOnline}`);
                
                // Check if we should update the status
                if (isEspOnline !== espOnline) {
                    console.log(`ESP32 status changed: ${espOnline ? 'Connected' : 'Disconnected'} -> ${isEspOnline ? 'Connected' : 'Disconnected'}`);
                    console.log(`Time since last ESP message: ${secondsSinceLastEspMsg.toFixed(1)}s`);
                    setEspOnline(isEspOnline);
                }
            }
        }, 5000); // Check every 5 seconds

        // Fetch recovery state on mount
        fetch('/api/recoveryState')
            .then((res) => res.json())
            .then((data) => {
                if (data && data.recoveryState) {
                    setRecoveryState(data.recoveryState);
                }
            })
            .catch((err) => console.error('Failed to fetch recovery state:', err));

        return () => {
            console.log('WebSocketProvider cleanup');
            mountedRef.current = false;
            
            // Clear intervals
            clearInterval(watchdogInterval);
            
            // Clear reconnection timeout
            if (reconnectTimeoutRef.current) {
                clearTimeout(reconnectTimeoutRef.current);
                reconnectTimeoutRef.current = null;
            }
            
            // Close connection cleanly
            if (socketRef.current) {
                socketRef.current.close(1000, 'Component unmounting');
                socketRef.current = null;
            }
            
            isConnectingRef.current = false;
            setIsConnected(false);
        };
    }, []);

    const value = {
        espOnline,
        recoveryState,
        currentTemp,
        espOutputs,
        sendParameters,
        sendButtonCommand,
        sendRecoveryUpdate,
        isConnected,
        sendMessage,
        resetRecoveryState,
    };

    return (
        <WebSocketContext.Provider value={value}>
            {children}
        </WebSocketContext.Provider>
    );
}

export function useWebSocket() {
    const context = useContext(WebSocketContext);
    if (!context) {
        throw new Error('useWebSocket must be used within a WebSocketProvider');
    }
    return context;
}
