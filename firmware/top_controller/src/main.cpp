#include <Arduino.h>

// This is the ESP-IDF entry point (Required!)
extern "C" void app_main() {
    initArduino(); // Start the Arduino engine
    
    Serial.begin(115200);
    Serial.println("MAIN CONTROLLER: Alive and waiting for code...");

    while (true) {
        // Just a heartbeat to show it hasn't crashed
        Serial.println("...tick...");
        delay(1000);
    }
}