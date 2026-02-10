#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

// --- PINS & CONFIGURATION ---
#define RXD2_PIN        4
#define TXD2_PIN        5
#define LED_PIN         2
#define UART_PORT_NUM   UART_NUM_2
#define BAUD_RATE       115200
#define BUF_SIZE        1024

// Tag for logging (looks professional in terminal)
static const char *TAG = "BOTTOM_CONTROLLER";

// --- INITIALIZATION FUNCTIONS ---

void init_led(void) {
    // Reset the pin first (good practice)
    gpio_reset_pin(LED_PIN);
    // Set direction to output
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    // Start OFF
    gpio_set_level(LED_PIN, 0);
}

void init_uart(void) {
    const uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // 1. Configure the UART parameters
    uart_param_config(UART_PORT_NUM, &uart_config);

    // 2. Set the pins (TX, RX, RTS, CTS)
    // We don't use flow control (RTS/CTS), so we pass UART_PIN_NO_CHANGE
    uart_set_pin(UART_PORT_NUM, TXD2_PIN, RXD2_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // 3. Install the driver
    // We need an RX buffer (BUF_SIZE * 2), but no TX buffer is strictly needed here.
    // 0 = queue size (we don't use event queue here), NULL = no queue handle, 0 = interrupt alloc flags
    uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    
    ESP_LOGI(TAG, "UART initialized on pins RX:%d TX:%d", RXD2_PIN, TXD2_PIN);
}

// --- TASK: THE LISTENER ---

void uart_rx_task(void *arg) {
    // Allocate a buffer on the heap for incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    ESP_LOGI(TAG, "Task started. Waiting for commands...");

    while (1) {
        // Read data from the UART
        // This blocks for up to 100ms (portTICK_PERIOD_MS) waiting for data
        int len = uart_read_bytes(UART_PORT_NUM, data, BUF_SIZE - 1, 20 / portTICK_PERIOD_MS);

        // If we received bytes
        if (len > 0) {
            // Null-terminate the data to make it a valid C-string
            data[len] = '\0';

            // Remove newline/carriage return characters (Manual "trim")
            // This modifies the 'data' buffer directly.
            char *pos;
            if ((pos = strchr((char *)data, '\n')) != NULL) *pos = '\0';
            if ((pos = strchr((char *)data, '\r')) != NULL) *pos = '\0';

            // LOGIC: Compare strings
            if (strcmp((char *)data, "turn_ON_led") == 0) {
                ESP_LOGI(TAG, "Command Received: LED ON");
                gpio_set_level(LED_PIN, 1);
            } 
            else if (strcmp((char *)data, "turn_OFF_led") == 0) {
                ESP_LOGI(TAG, "Command Received: LED OFF");
                gpio_set_level(LED_PIN, 0);
            }
            else {
                // If len > 1 just to avoid printing empty noises
                if(strlen((char*)data) > 0) {
                    ESP_LOGW(TAG, "Unknown Command: %s", data);
                }
            }
        }
    }
    // (Optional) free(data) if you ever break the loop
    free(data);
    vTaskDelete(NULL);
}

// --- MAIN ENTRY POINT ---

void app_main(void) {
    // 1. Initialize Hardware
    init_led();
    init_uart();

    // 2. Create the Task
    // Stack size 4096 bytes, Priority 5 (standard)
    xTaskCreate(uart_rx_task, "uart_rx_task", 4096, NULL, 5, NULL);
}