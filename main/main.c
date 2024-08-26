#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "main";

void init_nvs(void);

int app_main() {
  init_nvs();

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void init_nvs(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
}
