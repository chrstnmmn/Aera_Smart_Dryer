import React, { useState } from 'react';
import { StyleSheet, View } from 'react-native';
import { Provider as PaperProvider, Button, Text, Card, Title, Paragraph } from 'react-native-paper';
import { StatusBar } from 'expo-status-bar';

export default function App() {
  const [isLedOn, setIsLedOn] = useState(false);

  const toggleSwitch = () => {
    setIsLedOn(!isLedOn);
  };

  return (
    // 1. PROVIDER: Wraps the app to apply the Paper theme
    <PaperProvider>
      <View style={styles.container}>
        
        {/* CARD: A nice container for our controls */}
        <Card style={styles.card}>
          <Card.Content style={{alignItems: 'center'}}>
            <Title>LED Control</Title>
            <Paragraph>Tap the button to toggle the light</Paragraph>
            
            {/* THE STATUS TEXT */}
            <Text 
              variant="displayMedium" 
              style={{
                color: isLedOn ? '#4CAF50' : '#888', 
                fontWeight: 'bold', 
                marginVertical: 20
              }}
            >
              {isLedOn ? "ON" : "OFF"}
            </Text>

            {/* THE PAPER BUTTON */}
            {/* mode="contained" gives it the solid background fill */}
            <Button 
              icon={isLedOn ? "lightbulb-on" : "lightbulb-off"} 
              mode="contained" 
              onPress={toggleSwitch}
              buttonColor={isLedOn ? '#4CAF50' : '#333'}
              contentStyle={{ height: 60, width: 200 }} // Makes it big
              labelStyle={{ fontSize: 18, fontWeight: 'bold' }}
            >
              {isLedOn ? "TURN OFF" : "TURN ON"}
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
    backgroundColor: '#f5f5f5', // Light grey background
    alignItems: 'center',
    justifyContent: 'center',
  },
  card: {
    width: '90%',
    elevation: 4, // Adds shadow on Android
  }
});