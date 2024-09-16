#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "usb/cdc_acm_host.h"
#include "usb/vcp_ch34x.hpp"
#include "usb/vcp_cp210x.hpp"
#include "usb/vcp_ftdi.hpp"
#include "usb/vcp.hpp"
#include "usb/usb_host.h"
#include "cdc_wrapper.h"

using namespace esp_usb;

static const char *TAG = "VCP example";
static usb_cdc_wrapper_on_data_t s_on_data;
static volatile bool s_device_connected;
static SemaphoreHandle_t s_device_disconnected_sem;
static std::unique_ptr<CdcAcmDevice> s_vcp;

static bool handle_rx(const uint8_t *data, size_t data_len, void *arg)
{
    if (s_on_data) {
        s_on_data(data, data_len);
    }
    return true;
}

static void handle_event(const cdc_acm_host_dev_event_data_t *event, void *user_ctx)
{
    switch (event->type) {
    case CDC_ACM_HOST_ERROR:
        ESP_LOGE(TAG, "CDC-ACM error has occurred, err_no = %d", event->data.error);
        break;
    case CDC_ACM_HOST_DEVICE_DISCONNECTED:
        ESP_LOGI(TAG, "Device suddenly disconnected");
        xSemaphoreGive(s_device_disconnected_sem);
        s_device_connected = false;
        break;
    case CDC_ACM_HOST_SERIAL_STATE:
        ESP_LOGI(TAG, "Serial state notif 0x%04X", event->data.serial_state.val);
        break;
    case CDC_ACM_HOST_NETWORK_CONNECTION:
    default: break;
    }
}

static void usb_lib_task(void *arg)
{
    while (1) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            ESP_ERROR_CHECK(usb_host_device_free_all());
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            ESP_LOGI(TAG, "USB: All devices freed");
        }
    }
}

extern "C" esp_err_t usb_cdc_wrapper_init(usb_cdc_wrapper_on_data_t on_data)
{
    s_on_data = on_data;
    s_device_connected = false;
    s_device_disconnected_sem = xSemaphoreCreateBinary();
    ESP_LOGI(TAG, "Installing USB Host");
    usb_host_config_t host_config = {};
    host_config.skip_phy_setup = false;
    host_config.intr_flags = ESP_INTR_FLAG_LEVEL1;
    ESP_RETURN_ON_ERROR(usb_host_install(&host_config), TAG, "usb_host_install failed");

    BaseType_t task_created = xTaskCreate(usb_lib_task, "usb_lib", 4096, NULL, 10, NULL);
    ESP_RETURN_ON_FALSE(task_created, ESP_ERR_NO_MEM, TAG, "xTaskCreate failed");

    ESP_LOGI(TAG, "Installing CDC-ACM driver");
    ESP_RETURN_ON_ERROR(cdc_acm_host_install(NULL), TAG, "cdc_acm_host_install failed");

    VCP::register_driver<FT23x>();
    VCP::register_driver<CP210x>();
    VCP::register_driver<CH34x>();

    return ESP_OK;
}

extern "C" esp_err_t usb_cdc_wrapper_wait_for_device_connected(void)
{
    s_vcp.reset();
    const cdc_acm_host_device_config_t dev_config = {
        .connection_timeout_ms = 5000,
        .out_buffer_size = 512,
        .in_buffer_size = 512,
        .event_cb = handle_event,
        .data_cb = handle_rx,
        .user_arg = NULL,
    };
    ESP_LOGI(TAG, "Opening VCP device...");
    s_vcp = std::unique_ptr<CdcAcmDevice>(VCP::open(&dev_config));

    if (s_vcp == nullptr) {
        ESP_LOGI(TAG, "Failed to open VCP device");
        return ESP_FAIL;
    }
    vTaskDelay(10);

    ESP_LOGI(TAG, "Setting up line coding");
    cdc_acm_line_coding_t line_coding = {
        .dwDTERate = 115200,
        .bCharFormat = 0,  // 0: 1 stopbit, 1: 1.5 stopbits, 2: 2 stopbits
        .bParityType = 0,  // 0: None, 1: Odd, 2: Even, 3: Mark, 4: Space
        .bDataBits = 8,
    };
    ESP_RETURN_ON_ERROR(s_vcp->line_coding_set(&line_coding), TAG, "line_coding_set failed");
    s_device_connected = true;
    return ESP_OK;
}

esp_err_t usb_cdc_wrapper_wait_for_device_disconnected(void)
{
    xSemaphoreTake(s_device_disconnected_sem, portMAX_DELAY);
    return ESP_OK;
}

extern "C" esp_err_t usb_cdc_wrapper_set_baudrate(unsigned baudrate)
{
    if (!s_device_connected) {
        return ESP_ERR_INVALID_STATE;
    }
    ESP_LOGI(TAG, "Setting baud rate to %u", baudrate);
    cdc_acm_line_coding_t line_coding = {
        .dwDTERate = baudrate,
        .bCharFormat = 0,  // 0: 1 stopbit, 1: 1.5 stopbits, 2: 2 stopbits
        .bParityType = 0,  // 0: None, 1: Odd, 2: Even, 3: Mark, 4: Space
        .bDataBits = 8,
    };
    ESP_RETURN_ON_ERROR(s_vcp->line_coding_set(&line_coding), TAG, "line_coding_set failed");
    return ESP_OK;
}

extern "C" esp_err_t usb_cdc_wrapper_send_data(const uint8_t *data, size_t len)
{
    if (!s_device_connected) {
        return ESP_ERR_INVALID_STATE;
    }
    ESP_RETURN_ON_ERROR(s_vcp->tx_blocking((uint8_t *) data, len), TAG, "tx_blocking failed");
    return ESP_OK;
}

extern "C" esp_err_t usb_cdc_wrapper_set_line_control(bool dtr, bool rts)
{
    if (!s_device_connected) {
        return ESP_ERR_INVALID_STATE;
    }
    ESP_RETURN_ON_ERROR(s_vcp->set_control_line_state(dtr, rts), TAG, "set_control_line_state failed");
    return ESP_OK;
}
