#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*usb_cdc_wrapper_on_data_t)(const uint8_t *data, size_t len);

esp_err_t usb_cdc_wrapper_init(usb_cdc_wrapper_on_data_t on_data);
esp_err_t usb_cdc_wrapper_wait_for_device_connected(void);
esp_err_t usb_cdc_wrapper_wait_for_device_disconnected(void);
esp_err_t usb_cdc_wrapper_set_baudrate(unsigned baudrate);
esp_err_t usb_cdc_wrapper_set_line_control(bool dtr, bool rts);
esp_err_t usb_cdc_wrapper_send_data(const uint8_t *data, size_t len);


#ifdef __cplusplus
}
#endif
