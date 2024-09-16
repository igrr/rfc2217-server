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
#include "protocol_examples_common.h"
#include "sdkconfig.h"

#include "rfc2217_server.h"
#include "cdc_wrapper.h"

static const char *TAG = "app_main";
static rfc2217_server_t s_server;
static bool s_client_connected;
static bool s_dtr;
static bool s_rts;

static void on_connected(void *ctx);
static void on_disconnected(void *ctx);
static void on_data_received_from_rfc2217(void *ctx, const uint8_t *data, size_t len);
static void on_data_received_from_usb(const uint8_t *data, size_t len);
static unsigned on_baudrate(void *ctx, unsigned baudrate);
static rfc2217_control_t on_control(void *ctx, rfc2217_control_t requested_control);

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(example_connect());
    ESP_ERROR_CHECK(usb_cdc_wrapper_init(on_data_received_from_usb));

    rfc2217_server_config_t config = {
        .ctx = NULL,
        .on_client_connected = on_connected,
        .on_client_disconnected = on_disconnected,
        .on_baudrate = on_baudrate,
        .on_control = on_control,
        .on_purge = NULL,
        .on_data_received = on_data_received_from_rfc2217,
        .port = 3333,
        .task_stack_size = 4096,
        .task_priority = 5,
        .task_core_id = 0
    };

    ESP_ERROR_CHECK(rfc2217_server_create(&config, &s_server));

    ESP_LOGI(TAG, "Starting RFC2217 server on port %u", config.port);

    ESP_ERROR_CHECK(rfc2217_server_start(s_server));

    while (true) {
        usb_cdc_wrapper_wait_for_device_connected();
        ESP_LOGI(TAG, "USB Device connected");
        usb_cdc_wrapper_wait_for_device_disconnected();
        ESP_LOGI(TAG, "USB Device disconnected");
    }
}

static void on_connected(void *ctx)
{
    ESP_LOGI(TAG, "RFC2217 client connected");
    s_client_connected = true;
}

static void on_disconnected(void *ctx)
{
    ESP_LOGI(TAG, "RFC2217 client disconnected");
    s_client_connected = false;
}

static void on_data_received_from_rfc2217(void *ctx, const uint8_t *data, size_t len)
{
    usb_cdc_wrapper_send_data(data, len);
}

static void on_data_received_from_usb(const uint8_t *data, size_t len)
{
    if (!s_client_connected) {
        return;
    }
    rfc2217_server_send_data(s_server, data, len);
}

static unsigned on_baudrate(void *ctx, unsigned baudrate)
{
    usb_cdc_wrapper_set_baudrate(baudrate);
    return baudrate;
}

static rfc2217_control_t on_control(void *ctx, rfc2217_control_t requested_control)
{
    if (requested_control == RFC2217_CONTROL_SET_DTR) {
        s_dtr = true;
    } else if (requested_control == RFC2217_CONTROL_CLEAR_DTR) {
        s_dtr = false;
    } else if (requested_control == RFC2217_CONTROL_SET_RTS) {
        s_rts = true;
    } else if (requested_control == RFC2217_CONTROL_CLEAR_RTS) {
        s_rts = false;
    } else {
        return requested_control;
    }
    usb_cdc_wrapper_set_line_control(s_dtr, s_rts);
    return requested_control;
}
