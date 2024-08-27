#include "device_config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

#include "esp_now_communication.h"
#include "esp_now_pair.h"

device_config_t *device;

static const char *TAG = "device_config";

void device_register(device_config_t *device_to_register) {
  if (device_to_register != NULL) {
    device = device_to_register;
  } else {
    ESP_LOGE(TAG, "Failed to register device");
  }
}

void device_message_parse(const char *data) {
  for (int i = 0; i < DEVICE_MAX_COMMANDS; i++) {
    if (strlen(device->commands[i]) == 0) {
      // Skip empty (uninitialized) commands
      continue;
    }

    const char *command = device->commands[i];
    char *wildcard_pos = strchr(command, '*');

    if (wildcard_pos != NULL) {
      // Check the part before the wildcard
      int prefix_length = wildcard_pos - command;
      if (strncmp(data, command, prefix_length) == 0) {
        // If the part after the wildcard is non-empty, check that too
        const char *suffix = wildcard_pos + 1;
        if (strlen(suffix) == 0 ||
            strstr(data + prefix_length, suffix) != NULL) {
          // If the prefix and suffix match, or if the suffix is empty, trigger
          // the callback
          if (device->user_command_parser_cb != NULL) {
            device->user_command_parser_cb(data);
            return;
          }
        }
      }
    } else {
      // If no wildcard, check the entire command
      if (strncmp(data, command, DEVICE_COMMAND_MAX_SIZE) == 0) {
        if (device->user_command_parser_cb != NULL) {
          device->user_command_parser_cb(data);
          return;
        }
      }
    }
  }
}

char *device_generate_status_message(const char *status_fmt, ...) {
  if (status_fmt == NULL)
    status_fmt = device->status_fmt;

  va_list args;
  va_start(args, status_fmt);

  // Calculate the size needed for the formatted message
  int message_size =
      vsnprintf(NULL, 0, status_fmt, args) + 1; // +1 for the null terminator

  va_end(args);

  // Allocate memory for the message dynamically
  char *status_message = (char *)malloc(message_size);
  if (!status_message) {
    // Handle memory allocation failure
    return NULL;
  }

  // Reinitialize the argument list to format the message
  va_start(args, status_fmt);
  vsnprintf(status_message, message_size, status_fmt, args);
  va_end(args);

  return status_message;
}

char *device_generate_data_message(const char *data_fmt, ...) {
  if (data_fmt == NULL)
    data_fmt = device->data_fmt;

  va_list args;
  va_start(args, data_fmt);

  // Calculate the size needed for the formatted message
  int message_size =
      vsnprintf(NULL, 0, data_fmt, args) + 1; // +1 for the null terminator

  va_end(args);

  // Allocate memory for the message dynamically
  char *data_message = (char *)malloc(message_size);
  if (!data_message) {
    // Handle memory allocation failure
    return NULL;
  }

  // Reinitialize the argument list to format the message
  va_start(args, data_fmt);
  vsnprintf(data_message, message_size, data_fmt, args);
  va_end(args);

  return data_message;
}

char *device_get_pair_msg() {
  if (device->_pair_msg != NULL) {
    return device->_pair_msg;
  }

  if (strlen(device->config) == 0)
    sprintf(device->config, "{}");

  asprintf(&device->_pair_msg, PAIR_MSG_FMT, device->type, device->config);

  return device->_pair_msg;
}

bool device_send_status_msg() {
  char *status = device->user_status_msg_cb();

  if (status == NULL) {
    return false;
  }

  ESP_LOGI(TAG, "Sending status message: \"%s\"", status);

  bool ret = enc_send_with_result(status);
  free(status);
  return 1;
}

bool device_send_data_msg() {
  char *data = device->user_data_msg_cb();

  if (data == NULL) {
    return false;
  }

  ESP_LOGI(TAG, "Sending data message: \"%s\"", data);

  bool ret = enc_send_with_result(data);
  free(data);
  return 1;
}

void device_start(bool force_pair) {
  enc_init();
  enp_init(force_pair);
}