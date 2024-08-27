#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "link.h"

static const char *TAG = "main";

// Callback function triggered when a command is received
void on_command(const char *cmd) {
  ESP_LOGI(TAG, "Received command: %s", cmd);
  printf("\n\nCommand received: %s\n\n", cmd);

  // Send status message back after processing the command
  link_send_status_msg();
}

// Callback function to generate the status message
char *on_status_message(void) {
  char *status;
  // Example: checking the status of a lamp (uncomment if applicable)
  // if (lamp_is_on())
  //     status = link_generate_status_message(NULL, "ON", 100);
  // else
  status =
      link_generate_status_message(NULL, "OFF", 55); // Example status message
  return status;
}

// Callback function to generate the data message
char *on_data_message(void) {
  char *data;
  data = link_generate_data_message(
      NULL, 21.2,
      55.1); // Example data message (e.g., temperature and humidity)
  return data;
}

// Function to initialize Non-Volatile Storage (NVS)
void init_nvs(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
}

int app_main() {
  // Initialize NVS, required for storing pairing information
  init_nvs();

  // Define and configure the device using the link_config_t structure
  link_config_t my_device = {
      .type = 1, // Device type (1 = Temperature/Humidity Sensor)
      .config =
          "{\"data_interval\": 3000}", // Configuration sent during pairing
      .data_fmt = "D:{\"T\":%.2f, \"H\":%.2f}", // Data format to be sent
                                                // (Temperature, Humidity)
      .commands = {"ON", "OFF",
                   "SET_BRIGHTNESS=*"}, // Commands the device can respond to
      .status_fmt =
          "S:{\"state\":%s,\"brightness\":%u}", // Status format (e.g., ON/OFF
                                                // state, brightness level)
      .user_command_parser_cb =
          on_command, // Callback function for processing commands
      .user_status_msg_cb =
          on_status_message, // Callback function for generating status messages
      .user_data_msg_cb =
          on_data_message // Callback function for generating data messages
  };

  // Register the device with the link library
  link_register(&my_device);

  // Start the device, optionally forcing re-pairing
  link_start(false);

  // Main loop to periodically perform tasks (e.g., sending data)
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(5000));
    // Example: uncomment to send data messages periodically
    // link_send_data_msg();
  }
}
