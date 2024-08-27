idf_component_register(
    SRCS "esp_now_communication.c" "esp_now_pair.c" "link.c"
    INCLUDE_DIRS "."
    REQUIRES "esp_netif" "esp_wifi" "nvs_flash")
