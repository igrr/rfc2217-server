#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/semphr.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "rfc2217_server.h"

static const char *TAG = "app_main";
static rfc2217_server_t s_server;
static SemaphoreHandle_t s_client_connected;
static SemaphoreHandle_t s_client_disconnected;

static void on_connected(void *ctx);
static void on_disconnected(void *ctx);
static void on_data_received(void *ctx, const uint8_t *data, size_t len);

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(example_connect());

    s_client_connected = xSemaphoreCreateBinary();
    s_client_disconnected = xSemaphoreCreateBinary();

    rfc2217_server_config_t config = {
        .ctx = NULL,
        .on_client_connected = on_connected,
        .on_client_disconnected = on_disconnected,
        .on_baudrate = NULL,
        .on_control = NULL,
        .on_purge = NULL,
        .on_data_received = on_data_received,
        .port = 3333,
        .task_stack_size = 4096,
        .task_priority = 5,
        .task_core_id = 0
    };

    ESP_ERROR_CHECK(rfc2217_server_create(&config, &s_server));

    ESP_LOGI(TAG, "Starting RFC2217 server on port %u", config.port);

    ESP_ERROR_CHECK(rfc2217_server_start(s_server));

    while (true) {
        ESP_LOGI(TAG, "Waiting for client to connect");
        xSemaphoreTake(s_client_connected, portMAX_DELAY);

        ESP_LOGI(TAG, "Client connected, sending greeting");
        vTaskDelay(pdMS_TO_TICKS(1000));
        const char *msg = "\r\nHello from ESP RFC2217 server!\r\n";
        rfc2217_server_send_data(s_server, (const uint8_t *) msg, strlen(msg));

        ESP_LOGI(TAG, "Waiting for client to disconnect");
        xSemaphoreTake(s_client_disconnected, portMAX_DELAY);
        ESP_LOGI(TAG, "Client disconnected");
    }
}

static void on_connected(void *ctx)
{
    xSemaphoreGive(s_client_connected);
}

static void on_disconnected(void *ctx)
{
    xSemaphoreGive(s_client_disconnected);
}

static void on_data_received(void *ctx, const uint8_t *data, size_t len)
{
    ESP_LOGI(TAG, "Received %u byte(s)", (unsigned) len);
    // Echo back the received data
    rfc2217_server_send_data(s_server, data, len);
}
