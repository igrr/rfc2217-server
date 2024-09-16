idf_component_register(
    SRCS "usb_cdc_example_main.c" "cdc_wrapper.cpp"
    PRIV_INCLUDE_DIRS "."
    PRIV_REQUIRES esp_system lwip nvs_flash esp_netif esp_event)
