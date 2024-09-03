#include "link.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

#include "esp_now_communication.h"
#include "esp_now_pair.h"

link_config_t *link_device;

static const char *TAG = "Link";

typedef enum { LINK_MESSAGE_DATA, LINK_MESSAGE_STATUS } link_message_type_e;

void link_register(link_config_t *device_to_register) {
  if (device_to_register != NULL) {
    link_device = device_to_register;
  } else {
    ESP_LOGE(TAG, "Failed to register device");
  }
}

void link_message_parse(const char *data) {
  for (int i = 0; i < LINK_MAX_COMMANDS; i++) {
    if (strlen(link_device->commands[i]) == 0) {
      // Skip empty (uninitialized) commands
      continue;
    }

    const char *command = link_device->commands[i];
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
          if (link_device->user_command_parser_cb != NULL) {
            link_device->user_command_parser_cb(data);
            return;
          }
        }
      }
    } else {
      // If no wildcard, check the entire command
      if (strncmp(data, command, LINK_COMMAND_MAX_SIZE) == 0) {
        if (link_device->user_command_parser_cb != NULL) {
          link_device->user_command_parser_cb(data);
          return;
        }
      }
    }
  }
}

char *link_generate_status_message(const char *status_fmt, ...) {
  if (status_fmt == NULL)
    status_fmt = link_device->status_fmt;

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

char *link_generate_data_message(const char *data_fmt, ...) {
  if (data_fmt == NULL)
    data_fmt = link_device->data_fmt;

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

char *link_get_pair_msg() {
  if (link_device->_pair_msg != NULL) {
    return link_device->_pair_msg;
  }

  if (strlen(link_device->config) == 0)
    sprintf(link_device->config, "{}");

  asprintf(&link_device->_pair_msg, PAIR_MSG_FMT, link_device->type,
           link_device->config);

  return link_device->_pair_msg;
}

static inline bool link_send_msg(char *(*msg_cb)(),
                                 link_message_type_e msg_type) {
  if (msg_cb == NULL) {
    return false;
  }

  char *msg = msg_cb();

  if (msg == NULL) {
    return false;
  }

#if CONFIG_LINK_USE_PREFIX
  size_t prefix_len = (msg_type == LINK_MESSAGE_STATUS)
                          ? strlen(LINK_STATUS_PREFIX)
                          : strlen(LINK_DATA_PREFIX);

  size_t msg_len = strlen(msg);
  char *prefixed_msg = (char *)malloc(prefix_len + msg_len + 1);

  if (prefixed_msg == NULL) {
    free(msg);
    return false;
  }

  strcpy(prefixed_msg, (msg_type == LINK_MESSAGE_STATUS) ? LINK_STATUS_PREFIX
                                                         : LINK_DATA_PREFIX);
  strcat(prefixed_msg, msg);

  free(msg);
  msg = prefixed_msg;
#endif

  ESP_LOGD(TAG, "Sending %s message: \"%s\"",
           (msg_type == LINK_MESSAGE_STATUS) ? "status" : "data", msg);

  bool ret = enc_send_with_result(msg);
  free(msg);
  return ret;
}

bool link_send_status_msg() {
  return link_send_msg(link_device->user_status_msg_cb, LINK_MESSAGE_STATUS);
}

bool link_send_data_msg() {
  return link_send_msg(link_device->user_data_msg_cb, LINK_MESSAGE_DATA);
}

void link_start(bool force_pair) {
  enc_init();
  enp_init(force_pair);
}

void link_block_until_find_pair() {
  enp_block_until_find_pair();
}