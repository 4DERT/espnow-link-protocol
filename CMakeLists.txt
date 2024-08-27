set(COMPONENT_ADD_INCLUDEDIRS
    src
    )

file(GLOB SRCS
     src/*.c
     )

set(COMPONENT_SRCS ${SRCS})

set(COMPONENT_REQUIRES "esp_netif" "esp_wifi" "nvs_flash")

register_component()