/**
 * BOARD A: MAIN CONTROLLER
 * Stack: Hybrid (ESP-IDF + Arduino)
 */
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define RXD2 16
#define TXD2 17
#define LED_PIN 2

// --- TASK 1: THE COMMUNICATOR ---
void TaskUART(void *pvParameters)
{
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
    Serial.println("UART Task Started");

    for (;;)
    { // Infinite Loop
        Serial.print("Sending PING... ");
        Serial2.println("PING"); // Send to Board B

        // Wait for reply (Non-blocking)
        unsigned long start = millis();
        bool received = false;

        while (millis() - start < 1000)
        {
            if (Serial2.available())
            {
                String s = Serial2.readStringUntil('\n');
                s.trim();
                if (s == "PONG")
                {
                    Serial.println("Success! Got PONG.");
                    received = true;
                    break;
                }
            }
            vTaskDelay(10 / portTICK_PERIOD_MS); // Yield
        }

        if (!received)
            Serial.println("Failed. No reply.");

        vTaskDelay(2000 / portTICK_PERIOD_MS); // Wait 2 seconds
    }
}

// --- TASK 2: THE HEARTBEAT ---
void TaskBlink(void *pvParameters)
{
    pinMode(LED_PIN, OUTPUT);
    for (;;)
    {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Toggle LED
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

// --- STANDARD SETUP ---
void setup()
{
    Serial.begin(115200);

    // Run UART on Core 0 (Protocol CPU)
    xTaskCreatePinnedToCore(TaskUART, "UART", 4096, NULL, 1, NULL, 0);

    // Run Blink on Core 1 (Application CPU)
    xTaskCreatePinnedToCore(TaskBlink, "Blink", 2048, NULL, 1, NULL, 1);
}

void loop()
{
    // FreeRTOS has taken over. We can delete this task.
    vTaskDelete(NULL);
}

// --- HYBRID FIX: REQUIRED FOR LINKER ERROR ---
extern "C" void app_main()
{
    initArduino(); // Initialize the Arduino Framework
    setup();       // Run your setup
    // We don't need a loop() here because we used FreeRTOS tasks above
}