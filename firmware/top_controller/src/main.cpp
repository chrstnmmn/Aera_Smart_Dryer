/**
 * TOP CONTROLLER (UI Board)
 * Role: Wi-Fi WebSocket Server (STATIC IP) & UART Sender
 * STATUS LED: Fast Blink = Connecting | OFF = Connected
 */
#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// --- PINS ---
#define RXD2 4
#define TXD2 5
#define LED_PIN 2

// --- WIFI CREDENTIALS ---
const char *ssid = "HUAWEI-2.4G-ZxPH";
const char *password = "vq5hJkB8";

// --- STATIC IP SETTINGS ---
IPAddress local_IP(192, 168, 18, 200);
IPAddress gateway(192, 168, 18, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);

// --- GLOBAL FLAGS ---
// This variable tells TaskBlink what to do
volatile bool wifiConnected = false;

// --- WEBSOCKET SERVER ---
WebSocketsServer webSocket = WebSocketsServer(81);

// --- HELPER: UART SENDER ---
void sendToBottom(String command)
{
    Serial.print("UART >> ");
    Serial.println(command);
    Serial2.println(command);
}

// --- WEBSOCKET EVENT HANDLER ---
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    String text = "";
    switch (type)
    {
    case WStype_DISCONNECTED:
        Serial.printf("[%u] Disconnected!\n", num);
        break;
    case WStype_CONNECTED:
        Serial.printf("[%u] Connected!\n", num);
        webSocket.sendTXT(num, "Connected to Aera Dryer");
        break;
    case WStype_TEXT:
        text = (char *)payload;
        Serial.printf("[%u] Received: %s\n", num, text.c_str());
        if (text == "ON")
        {
            sendToBottom("turn_ON_led");
            webSocket.broadcastTXT("STATUS:ON");
        }
        else if (text == "OFF")
        {
            sendToBottom("turn_OFF_led");
            webSocket.broadcastTXT("STATUS:OFF");
        }
        else if (text == "PING")
        {
            webSocket.sendTXT(num, "PONG");
        }
        break;
    default:
        break;
    }
}

// --- TASK: WIFI & WEBSOCKET ---
void TaskNet(void *pvParameters)
{
    // 1. FORCE STATIC IP
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS))
    {
        Serial.println("Static IP Failed to configure!");
    }

    // 2. Connect
    WiFi.begin(ssid, password);
    Serial.print("Connecting to Wi-Fi");

    // Loop until connected
    while (WiFi.status() != WL_CONNECTED)
    {
        wifiConnected = false; // Tell Blink Task to go crazy
        delay(500);
        Serial.print(".");
    }

    // 3. Success!
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    wifiConnected = true; // Tell Blink Task to STOP

    // 4. Start WebSocket
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    for (;;)
    {
        // If we lose connection, update the flag so blinking starts again
        if (WiFi.status() != WL_CONNECTED)
        {
            wifiConnected = false;
        }
        else
        {
            wifiConnected = true;
        }

        webSocket.loop();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

// --- TASK: BLINK (Status Indicator) ---
void TaskBlink(void *pvParameters)
{
    pinMode(LED_PIN, OUTPUT);

    for (;;)
    {
        if (wifiConnected)
        {
            // CASE A: CONNECTED -> LED OFF (Chill Mode)
            digitalWrite(LED_PIN, LOW);
            vTaskDelay(500 / portTICK_PERIOD_MS); // Check again in 0.5s
        }
        else
        {
            // CASE B: CONNECTING -> FAST BLINK (Panic Mode)
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            vTaskDelay(100 / portTICK_PERIOD_MS); // 100ms = Very Fast
        }
    }
}

void setup()
{
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);

    xTaskCreatePinnedToCore(TaskNet, "Net", 8192, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(TaskBlink, "Blink", 2048, NULL, 1, NULL, 1);
}

void loop() { vTaskDelete(NULL); }

extern "C" void app_main()
{
    initArduino();
    setup();
}