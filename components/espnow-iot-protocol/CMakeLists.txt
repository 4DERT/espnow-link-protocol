idf_component_register(
    SRCS "esp_now_communication.c" 
    INCLUDE_DIRS "."
    REQUIRES "esp_netif" "esp_wifi" "nvs_flash")
