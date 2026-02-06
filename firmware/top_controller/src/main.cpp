#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define RXD2 4
#define TXD2 5
#define LED_PIN 2

// --- TASK 1: THE COMMUNICATOR ---
void TaskUART(void *pvParameters)
{
    // 1. Setup Serial2 with a safety timeout!
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
    Serial2.setTimeout(50); // <--- CRITICAL FIX: Only block for 50ms, not 1000ms

    Serial.println("UART Task Started");

    for (;;)
    {
        Serial.print("Sending PING... ");
        // Clear buffer before sending to avoid reading old junk
        while (Serial2.available())
            Serial2.read();

        Serial2.println("PING");

        // Wait for reply safely
        unsigned long start = millis();
        bool received = false;

        while (millis() - start < 1000)
        {
            if (Serial2.available())
            {
                // This line used to crash the system. Now it's safe.
                String s = Serial2.readStringUntil('\n');
                s.trim();

                if (s == "PONG")
                {
                    Serial.println("Success! Got PONG.");
                    received = true;
                    break;
                }
            }
            // Vital: Give the CPU back to the OS for 10ms
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }

        if (!received)
            Serial.println("Failed. No reply.");

        // Wait 2 seconds before next ping
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void TaskBlink(void *pvParameters)
{
    pinMode(LED_PIN, OUTPUT);
    for (;;)
    {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void setup()
{
    Serial.begin(115200);
    xTaskCreatePinnedToCore(TaskUART, "UART", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(TaskBlink, "Blink", 2048, NULL, 1, NULL, 1);
}

void loop() { vTaskDelete(NULL); }

extern "C" void app_main()
{
    initArduino();
    setup();
}