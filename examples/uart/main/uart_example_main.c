#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "esp_err.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_intr_alloc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "protocol_examples_common.h"
#include "sdkconfig.h"

#include "rfc2217_server.h"

static const char *TAG = "app_main";
static rfc2217_server_t s_server;
static SemaphoreHandle_t s_client_connected;
static SemaphoreHandle_t s_client_disconnected;

static void on_connected(void *ctx);
static void on_disconnected(void *ctx);
static void on_data_received(void *ctx, const uint8_t *data, size_t len);
static unsigned on_baudrate(void *ctx, unsigned baudrate);

static esp_err_t init_uart(void);

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(example_connect());
    ESP_ERROR_CHECK(init_uart());

    s_client_connected = xSemaphoreCreateBinary();
    s_client_disconnected = xSemaphoreCreateBinary();

    rfc2217_server_config_t config = {
        .ctx = NULL,
        .on_client_connected = on_connected,
        .on_client_disconnected = on_disconnected,
        .on_baudrate = on_baudrate,
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

        ESP_LOGI(TAG, "Client connected, starting data transfer");

        while (xSemaphoreTake(s_client_disconnected, 0) != pdTRUE) {
            static uint8_t uart_read_buf[2048];
            int len = uart_read_bytes(CONFIG_EXAMPLE_UART_PORT_NUM, uart_read_buf, sizeof(uart_read_buf), pdMS_TO_TICKS(100));
            if (len > 0) {
                rfc2217_server_send_data(s_server, uart_read_buf, len);
            }
        }

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
    uart_write_bytes(CONFIG_EXAMPLE_UART_PORT_NUM, data, len);
}

static esp_err_t init_uart(void)
{
    // Initial config. It may be changed later according to the RFC2217 negotiation.
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    const size_t tx_buffer_size = 4096;
    const size_t rx_buffer_size = 4096;
    ESP_RETURN_ON_ERROR(uart_driver_install(CONFIG_EXAMPLE_UART_PORT_NUM, rx_buffer_size, tx_buffer_size, 0, NULL, ESP_INTR_FLAG_LOWMED), TAG, "uart_driver_install failed");

    ESP_RETURN_ON_ERROR(uart_param_config(CONFIG_EXAMPLE_UART_PORT_NUM, &uart_config), TAG, "uart_param_config failed");

    ESP_RETURN_ON_ERROR(uart_set_pin(CONFIG_EXAMPLE_UART_PORT_NUM, CONFIG_EXAMPLE_UART_TX_GPIO, CONFIG_EXAMPLE_UART_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE), TAG, "uart_set_pin failed");

    return ESP_OK;
}

static unsigned on_baudrate(void *ctx, unsigned baudrate)
{
    esp_err_t err = uart_set_baudrate(CONFIG_EXAMPLE_UART_PORT_NUM, baudrate);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set baudrate: %u", baudrate);
        return 0;
    }
    return baudrate;
}
