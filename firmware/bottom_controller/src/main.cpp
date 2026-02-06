/**
 * BOTTOM CONTROLLER (Main Board)
 * Role: Listener (Ponger)
 * Actions: Waits for PING from Top, replies PONG.
 */
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define RXD2 4
#define TXD2 5
#define LED_PIN 2

// --- TASK 1: THE LISTENER ---
void TaskUART(void *pvParameters)
{
    // 1. Setup Serial2
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
    Serial2.setTimeout(50); // Safety timeout
    
    Serial.println("UART Task Started (Listening Mode)");

    for (;;)
    {
        // 2. Check if data is available
        if (Serial2.available())
        {
            String s = Serial2.readStringUntil('\n');
            s.trim(); // Remove whitespace/newlines

            // 3. If Top says "PING", we say "PONG"
            if (s == "PING")
            {
                Serial.println("Heard PING! Replying PONG...");
                Serial2.println("PONG");
            }
            else if (s.length() > 0)
            {
                // Debugging: Print any weird garbage we receive
                Serial.print("Received Unknown: ");
                Serial.println(s);
            }
        }

        // CRITICAL: Yield to Watchdog Timer (WDT)
        // If no data, this lets the CPU rest.
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// --- TASK 2: THE HEARTBEAT ---
void TaskBlink(void *pvParameters)
{
    pinMode(LED_PIN, OUTPUT);
    for (;;)
    {
        // Blink fast (200ms) to distinguish from Top board
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

// --- STANDARD SETUP ---
void setup()
{
    Serial.begin(115200);
    
    // Core 0 is usually busy with Wi-Fi, so we can put UART there if Wi-Fi isn't heavy yet.
    // Or stick to Core 1 (App Core) to be safe from WDT resets.
    xTaskCreatePinnedToCore(TaskUART, "UART", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(TaskBlink, "Blink", 2048, NULL, 1, NULL, 1);
}

void loop() { vTaskDelete(NULL); }

// --- HYBRID FIX ---
extern "C" void app_main()
{
    initArduino();
    setup();
}