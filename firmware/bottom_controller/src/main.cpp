/**
 * BOARD B: UI CONTROLLER
 * Stack: Hybrid (ESP-IDF + Arduino)
 */
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define RXD2 16
#define TXD2 17
#define LED_PIN 2

// --- TASK 1: THE LISTENER ---
void TaskUART(void *pvParameters)
{
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
    Serial.println("Listening for PING...");

    for (;;)
    {
        if (Serial2.available())
        {
            String s = Serial2.readStringUntil('\n');
            s.trim();

            if (s == "PING")
            {
                Serial.println("Heard PING! Replying PONG...");
                Serial2.println("PONG");
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); // Yield CPU
    }
}

// --- TASK 2: THE HEARTBEAT ---
void TaskBlink(void *pvParameters)
{
    pinMode(LED_PIN, OUTPUT);
    for (;;)
    {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        // Fast blink (200ms) to distinguish from Board A
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

// --- STANDARD SETUP ---
void setup()
{
    Serial.begin(115200);
    xTaskCreatePinnedToCore(TaskUART, "UART", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(TaskBlink, "Blink", 2048, NULL, 1, NULL, 1);
}

void loop()
{
    vTaskDelete(NULL);
}

// --- HYBRID FIX: REQUIRED FOR LINKER ERROR ---
extern "C" void app_main()
{
    initArduino();
    setup();
}