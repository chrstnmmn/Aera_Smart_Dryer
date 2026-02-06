import React, { useState, useEffect, useRef } from 'react';
import { StyleSheet, View } from 'react-native';
import { Provider as PaperProvider, Button, Text, Card, Title, Paragraph } from 'react-native-paper';
import { StatusBar } from 'expo-status-bar';

export default function App() {
  const [isLedOn, setIsLedOn] = useState(false);
  const [isConnected, setIsConnected] = useState(false);
  const [statusText, setStatusText] = useState("Connecting...");

  // --- REFS ---
  const ws = useRef(null);
  const reconnectTimeout = useRef(null);
  const watchdogTimer = useRef(null); // Timer to check for death
  const lastPongTime = useRef(Date.now()); // Timestamp of last message

  // --- HEARTBEAT FUNCTION ---
  // This keeps the connection alive and detects death
  const startWatchdog = () => {
    // Clear existing timer if any
    if (watchdogTimer.current) clearInterval(watchdogTimer.current);

    // Reset the "last heard from server" time
    lastPongTime.current = Date.now();

    watchdogTimer.current = setInterval(() => {
      if (ws.current && ws.current.readyState === WebSocket.OPEN) {

        // 1. Send PING to ESP32
        try {
          ws.current.send("PING");
        } catch (e) {
          console.log("Ping failed");
        }

        // 2. Check if ESP32 is dead (No reply for 4 seconds)
        const timeSinceLastPong = Date.now() - lastPongTime.current;
        if (timeSinceLastPong > 4000) {
          console.log("💀 DEAD CONNECTION DETECTED! (Timeout)");
          ws.current.close(); // This triggers onclose -> reconnect
        }
      }
    }, 1500); // Check every 1.5 seconds
  };

  // --- CONNECT FUNCTION ---
  const connect = () => {
    if (ws.current && (ws.current.readyState === WebSocket.OPEN || ws.current.readyState === WebSocket.CONNECTING)) {
      return;
    }

    setStatusText("Connecting...");

    // CHANGE IP IF NEEDED
    ws.current = new WebSocket('ws://192.168.18.200:81');

    ws.current.onopen = () => {
      console.log('WebSocket Connected');
      setIsConnected(true);
      setStatusText("Online");

      if (reconnectTimeout.current) clearTimeout(reconnectTimeout.current);

      // Start the heartbeat when we connect
      startWatchdog();
    };

    ws.current.onclose = () => {
      console.log('WebSocket Disconnected');
      setIsConnected(false);
      setStatusText("Disconnected. Retrying...");

      // Stop the watchdog so we don't keep pinging a dead socket
      if (watchdogTimer.current) clearInterval(watchdogTimer.current);

      // Try to reconnect in 3 seconds
      reconnectTimeout.current = setTimeout(() => {
        connect();
      }, 3000);
    };

    ws.current.onerror = (e) => {
      // Just let onclose handle it
    };

    ws.current.onmessage = (e) => {
      // 3. We heard from the Server! Reset the death timer.
      lastPongTime.current = Date.now();

      if (e.data === "STATUS:ON") setIsLedOn(true);
      if (e.data === "STATUS:OFF") setIsLedOn(false);
      // We ignore "PONG" messages here, they just update lastPongTime above
    };
  };

  useEffect(() => {
    connect();
    return () => {
      if (ws.current) ws.current.close();
      if (reconnectTimeout.current) clearTimeout(reconnectTimeout.current);
      if (watchdogTimer.current) clearInterval(watchdogTimer.current);
    };
  }, []);

  const toggleSwitch = () => {
    if (!isConnected) return;
    const command = !isLedOn ? "ON" : "OFF";
    ws.current.send(command);
  };

  return (
    <PaperProvider>
      <View style={styles.container}>
        <Card style={styles.card}>
          <Card.Content style={{ alignItems: 'center' }}>
            <Title>Aera Smart Control</Title>

            <Paragraph style={{
              color: isConnected ? 'green' : (statusText.includes("Retrying") ? 'orange' : 'red')
            }}>
              Status: {statusText}
            </Paragraph>

            <Text variant="displayMedium" style={{ color: isLedOn ? '#4CAF50' : '#888', fontWeight: 'bold', marginVertical: 20 }}>
              {isLedOn ? "RUNNING" : "STOPPED"}
            </Text>

            <Button
              icon={isLedOn ? "fan" : "fan-off"}
              mode="contained"
              onPress={toggleSwitch}
              buttonColor={isLedOn ? '#4CAF50' : '#333'}
              contentStyle={{ height: 60, width: 200 }}
              labelStyle={{ fontSize: 18, fontWeight: 'bold' }}
              disabled={!isConnected}
            >
              {isLedOn ? "STOP" : "START"}
            </Button>

          </Card.Content>
        </Card>
        <StatusBar style="auto" />
      </View>
    </PaperProvider>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f5f5f5', alignItems: 'center', justifyContent: 'center' },
  card: { width: '90%', elevation: 4 }
});