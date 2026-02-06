import React, { useState, useEffect, useRef } from 'react';
import { StyleSheet, View } from 'react-native';
import { Provider as PaperProvider, Button, Text, Card, Title, Paragraph } from 'react-native-paper';
import { StatusBar } from 'expo-status-bar';

export default function App() {
  const [isLedOn, setIsLedOn] = useState(false);
  const [isConnected, setIsConnected] = useState(false);
  const [statusText, setStatusText] = useState("Connecting...");

  // Ref to keep the socket object alive
  const ws = useRef(null);

  useEffect(() => {
    // 1. Create WebSocket Connection
    // CHANGE THIS IP ADDRESS!
    ws.current = new WebSocket('ws://192.168.18.200:81');

    ws.current.onopen = () => {
      console.log('WebSocket Connected');
      setIsConnected(true);
      setStatusText("Online");
    };

    ws.current.onclose = () => {
      console.log('WebSocket Disconnected');
      setIsConnected(false);
      setStatusText("Disconnected");
    };

    ws.current.onerror = (e) => {
      console.log('WebSocket Error', e);
      setStatusText("Error");
    };

    ws.current.onmessage = (e) => {
      console.log("Received: ", e.data);
      // Update UI if the ESP32 tells us the status changed
      if (e.data === "STATUS:ON") setIsLedOn(true);
      if (e.data === "STATUS:OFF") setIsLedOn(false);
    };

    // Cleanup when app closes
    return () => {
      ws.current.close();
    };
  }, []);

  const toggleSwitch = () => {
    if (!isConnected) {
      alert("Not connected to Dryer!");
      return;
    }

    const command = !isLedOn ? "ON" : "OFF";

    // 2. Send Message instantly
    ws.current.send(command);

    // We wait for the ESP32 to confirm via 'onmessage' before updating state
    // But for a snappy feel, we can update mostly now:
    // setIsLedOn(!isLedOn); 
  };

  return (
    <PaperProvider>
      <View style={styles.container}>
        <Card style={styles.card}>
          <Card.Content style={{ alignItems: 'center' }}>
            <Title>Aera Smart Control</Title>
            <Paragraph style={{ color: isConnected ? 'green' : 'red' }}>
              Status: {statusText}
            </Paragraph>

            <Text
              variant="displayMedium"
              style={{
                color: isLedOn ? '#4CAF50' : '#888',
                fontWeight: 'bold',
                marginVertical: 20
              }}
            >
              {isLedOn ? "RUNNING" : "STOPPED"}
            </Text>

            <Button
              icon={isLedOn ? "fan" : "fan-off"}
              mode="contained"
              onPress={toggleSwitch}
              buttonColor={isLedOn ? '#4CAF50' : '#333'}
              contentStyle={{ height: 60, width: 200 }}
              labelStyle={{ fontSize: 18, fontWeight: 'bold' }}
              disabled={!isConnected} // Disable button if offline
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
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
    alignItems: 'center',
    justifyContent: 'center',
  },
  card: {
    width: '90%',
    elevation: 4,
  }
});