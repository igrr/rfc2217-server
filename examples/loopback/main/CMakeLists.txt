idf_component_register(
    SRCS "app_main.c"
    PRIV_INCLUDE_DIRS "."
    PRIV_REQUIRES esp_system lwip nvs_flash esp_netif esp_event)
