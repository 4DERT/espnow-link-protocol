#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "device_config.h"

static const char *TAG = "main";

void on_command(const char *cmd) {
  ESP_LOGI(TAG, "on_device_command_cb: %s", cmd);

  printf("\n\n ODEBRANO\n%s\n\n", cmd);

}

char *on_status_message(void) {
  return NULL;
}

char *on_data_message(void) {
  char *status;
  status = device_generate_data_message(NULL, 21.2, 55.1);
  return status;
}

void init_nvs(void);

int app_main() {
  init_nvs();

  device_config_t my_device = {
      .type = 1,
      .config = "{\"data_interval\": 3000}",
      .data_fmt = "D:{\"T\":%.2f, \"H\":%.2f}",
      .commands = {"ON", "OFF", "SET_BRIGHTNESS=*"},
      .user_command_parser_cb = on_command,
      .user_status_msg_cb = on_status_message,
      .user_data_msg_cb = on_data_message
  };

  device_register(&my_device);

  device_start(false);

  // ESP_LOGI(TAG, "\n %s \n", device_get_pair_msg());
  // device_send_data_msg();


  while (1) {
    vTaskDelay(pdMS_TO_TICKS(5000));
    // device_send_data_msg();
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
