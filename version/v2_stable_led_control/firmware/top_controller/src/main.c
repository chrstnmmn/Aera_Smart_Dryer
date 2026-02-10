#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_http_server.h"

// --- CONFIGURATION ---
#define WIFI_SSID "HUAWEI-2.4G-ZxPH"
#define WIFI_PASS "vq5hJkB8"
#define SERVER_PORT 81

// --- STATIC IP CONFIG ---
#define STATIC_IP_ADDR "192.168.18.200"
#define STATIC_GW_ADDR "192.168.18.1"
#define STATIC_NM_ADDR "255.255.255.0"

// --- PINS ---
#define LED_PIN 2
#define TXD2_PIN 5
#define RXD2_PIN 4
#define UART_PORT_NUM UART_NUM_2

// --- EVENT GROUP BITS ---
// We use these bits to signal state between tasks safely
#define WIFI_CONNECTED_BIT BIT0

static const char *TAG = "TOP_CONTROLLER";
static EventGroupHandle_t s_wifi_event_group;
static httpd_handle_t server = NULL;

// --- UART SENDER HELPER ---
void send_uart_command(const char *command)
{
    // Send string + newline
    uart_write_bytes(UART_PORT_NUM, command, strlen(command));
    uart_write_bytes(UART_PORT_NUM, "\n", 1);
    ESP_LOGI(TAG, "Sent UART: %s", command);
}

// --- WEBSOCKET HANDLER ---
// This function handles the WebSocket data frames
static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "Handshake done, WebSocket connection established");
        return ESP_OK;
    }

    // 1. Determine packet size
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    // First call with buffer=NULL to get the length
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK)
        return ret;

    if (ws_pkt.len > 0)
    {
        // 2. Allocate memory and read data
        uint8_t *buf = calloc(1, ws_pkt.len + 1);
        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);

        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "WS Received: %s", ws_pkt.payload);

            // 3. LOGIC: Handle Commands
            if (strcmp((char *)ws_pkt.payload, "ON") == 0)
            {
                send_uart_command("turn_ON_led");

                // Broadcast status back (Simple echo to sender for now)
                httpd_ws_frame_t resp_pkt;
                memset(&resp_pkt, 0, sizeof(httpd_ws_frame_t));
                resp_pkt.payload = (uint8_t *)"STATUS:ON";
                resp_pkt.len = 9;
                resp_pkt.type = HTTPD_WS_TYPE_TEXT;
                httpd_ws_send_frame(req, &resp_pkt);
            }
            else if (strcmp((char *)ws_pkt.payload, "OFF") == 0)
            {
                send_uart_command("turn_OFF_led");

                httpd_ws_frame_t resp_pkt;
                memset(&resp_pkt, 0, sizeof(httpd_ws_frame_t));
                resp_pkt.payload = (uint8_t *)"STATUS:OFF";
                resp_pkt.len = 10;
                resp_pkt.type = HTTPD_WS_TYPE_TEXT;
                httpd_ws_send_frame(req, &resp_pkt);
            }
            else if (strcmp((char *)ws_pkt.payload, "PING") == 0)
            {
                httpd_ws_frame_t resp_pkt;
                memset(&resp_pkt, 0, sizeof(httpd_ws_frame_t));
                resp_pkt.payload = (uint8_t *)"PONG";
                resp_pkt.len = 4;
                resp_pkt.type = HTTPD_WS_TYPE_TEXT;
                httpd_ws_send_frame(req, &resp_pkt);
            }
        }
        free(buf);
    }
    return ret;
}

// --- SERVER INIT ---
static void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = SERVER_PORT; // Set to 81 as per request

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Register URI handler
        httpd_uri_t ws_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = ws_handler,
            .user_ctx = NULL,
            .is_websocket = true};
        httpd_register_uri_handler(server, &ws_uri);
    }
}

// --- WIFI EVENT HANDLER ---
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        // Clear the Connected Bit
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        esp_wifi_connect();
        ESP_LOGI(TAG, "Retry to connect to the AP");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        // Set the Connected Bit
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        start_webserver();
    }
}

// --- INITIALIZERS ---
void init_uart()
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, TXD2_PIN, RXD2_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT_NUM, 1024, 0, 0, NULL, 0);
}

void init_wifi_static_ip()
{
    s_wifi_event_group = xEventGroupCreate();

    // Initialize networking stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create Default Wi-Fi Station
    esp_netif_t *my_sta = esp_netif_create_default_wifi_sta();

    // --- STATIC IP SETUP ---
    // 1. Stop DHCP Client
    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(my_sta));

    // 2. Set IP Info
    esp_netif_ip_info_t ip_info;
    esp_netif_str_to_ip4(STATIC_IP_ADDR, &ip_info.ip);
    esp_netif_str_to_ip4(STATIC_GW_ADDR, &ip_info.gw);
    esp_netif_str_to_ip4(STATIC_NM_ADDR, &ip_info.netmask);
    ESP_ERROR_CHECK(esp_netif_set_ip_info(my_sta, &ip_info));

    // 3. Set DNS (Google DNS)
    esp_netif_dns_info_t dns_info;
    esp_netif_str_to_ip4("8.8.8.8", &dns_info.ip.u_addr.ip4);
    ESP_ERROR_CHECK(esp_netif_set_dns_info(my_sta, ESP_NETIF_DNS_MAIN, &dns_info));

    // Standard Wi-Fi Init
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

// --- TASK: LED STATUS ---
void status_led_task(void *pvParameters)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    while (1)
    {
        // Check if Connected Bit is set
        EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);

        if (bits & WIFI_CONNECTED_BIT)
        {
            // Connected: LED OFF (High logic or Low logic depending on your board, assuming LOW = OFF)
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        else
        {
            // Connecting: Blink Fast (100ms)
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}

// --- MAIN ---
void app_main(void)
{
    // Initialize NVS (Required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    init_uart();
    init_wifi_static_ip(); // This triggers the connection process

    // Create the LED task
    xTaskCreate(status_led_task, "led_task", 2048, NULL, 5, NULL);
}