/**
 * BOTTOM CONTROLLER (Main Board)
 * Role: ACTOR
 * Action: Receives "turn_ON_led" -> Turns Pin 2 HIGH
 */
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// --- PINS (Matches your setup) ---
#define RXD2 4
#define TXD2 5
#define LED_PIN 2

// --- TASK: THE LISTENER ---
void TaskUART(void *pvParameters)
{
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
    Serial2.setTimeout(50);

    // Set LED to Output and start OFF
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.println("UART Task Started. Waiting for commands...");

    for (;;)
    {
        if (Serial2.available())
        {
            String s = Serial2.readStringUntil('\n');
            s.trim();

            // LOGIC: Check for specific commands
            if (s == "turn_ON_led")
            {
                Serial.println("Command: LED ON");
                digitalWrite(LED_PIN, HIGH); // Turn ON and keep it ON
            }
            else if (s == "turn_OFF_led")
            {
                Serial.println("Command: LED OFF");
                digitalWrite(LED_PIN, LOW); // Turn OFF and keep it OFF
            }
            else if (s.length() > 0)
            {
                Serial.print("Unknown Command: ");
                Serial.println(s);
            }
        }

        // Essential delay to prevent Watchdog Crash
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void setup()
{
    Serial.begin(115200);

    // We only run the UART task now.
    // I removed TaskBlink so it doesn't fight for control of the LED.
    xTaskCreatePinnedToCore(TaskUART, "UART", 4096, NULL, 1, NULL, 1);
}

void loop() { vTaskDelete(NULL); }

extern "C" void app_main()
{
    initArduino();
    setup();
}