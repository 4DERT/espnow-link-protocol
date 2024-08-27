idf_component_register(
    SRCS "esp_now_communication.c" "esp_now_pair.c" "device_config.c"
    INCLUDE_DIRS "."
    REQUIRES "esp_netif" "esp_wifi" "nvs_flash")
