#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "rfc2217_server.h"

static const char *TAG = "app_main";
static volatile bool client_connected;

static void on_connected(void *ctx)
{
    ESP_LOGI(TAG, "Client connected");
    client_connected = true;
}
static void on_disconnected(void *ctx)
{
    ESP_LOGI(TAG, "Client disconnected");
    client_connected = false;
}
static void on_data_received(void *ctx, const uint8_t *data, size_t len)
{
    ESP_LOGI(TAG, "Data received: (%u bytes)", (unsigned) len);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(example_connect());

    rfc2217_server_config_t config = {
        .ctx = NULL,
        .on_client_connected = on_connected,
        .on_client_disconnected = on_disconnected,
        .on_baudrate = NULL,
        .on_control = NULL,
        .on_purge = NULL,
        .on_data_received = on_data_received,
        .port = 3333,
        .max_clients = 1,
        .task_stack_size = 4096,
        .task_priority = 5,
        .task_core_id = 0
    };

    rfc2217_server_t server;
    ESP_ERROR_CHECK(rfc2217_server_create(&config, &server));
    ESP_ERROR_CHECK(rfc2217_server_start(server));
    ESP_LOGI(TAG, "RFC2217 server started");

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if (client_connected) {
            const char *msg = "Hello, RFC2217 client!\r\n";
            rfc2217_server_send_data(server, (const uint8_t *) msg, strlen(msg));
        }
    }

}
