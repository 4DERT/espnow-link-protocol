#ifndef LINK_CONFIG_H_
#define LINK_CONFIG_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "sdkconfig.h"

#define PAIR_MSG_FMT "SHPR:{\"type\":%d,\"cfg\":%s}"
#define PAIR_ACCEPT "SHPR:PAIRED"

#define LINK_CONFIG_SIZE CONFIG_LINK_CONFIG_SIZE
#define LINK_STATUS_FMT_SIZE CONFIG_LINK_STATUS_FMT_SIZE
#define LINK_DATA_FMT_SIZE CONFIG_LINK_DATA_FMT_SIZE

#define LINK_MAX_COMMANDS CONFIG_LINK_MAX_COMMANDS
#define LINK_COMMAND_MAX_SIZE CONFIG_LINK_COMMAND_MAX_SIZE

#define LINK_USE_PREFIX CONFIG_LINK_USE_PREFIX
#define LINK_STATUS_PREFIX "!S:"
#define LINK_DATA_PREFIX "!D:"

/**
 * @brief Callback type for handling received commands.
 *
 * @param cmd The received command string.
 */
typedef void (*link_command_cb)(const char *cmd);

/**
 * @brief Callback type for generating the status message.
 *
 * @return A dynamically allocated string containing the status message.
 */
typedef char *(*link_status_message_cb)(void);

/**
 * @brief Callback type for generating the data message.
 *
 * @return A dynamically allocated string containing the data message.
 */
typedef char *(*link_data_message_cb)(void);

/**
 * @brief Structure representing the device configuration.
 */
typedef struct {
  /**
   * Device type identifier, used during pairing to indicate the type of device.
   */
  int type;

  /**
   * JSON formatted configuration string sent during pairing.
   * Leave empty if not used.
   */
  char config[LINK_CONFIG_SIZE];

  /**
   * Format string for the device status message.
   */
  char status_fmt[LINK_STATUS_FMT_SIZE];

  /**
   * Format string for the device data message.
   */
  char data_fmt[LINK_DATA_FMT_SIZE];

  /**
   * Array of commands that the device can handle.
   * Use '*' to denote a wildcard in commands.
   */
  char commands[LINK_MAX_COMMANDS][LINK_COMMAND_MAX_SIZE];

  /**
   * User-defined callback function that is called when a command from
   * the commands array is received.
   */
  link_command_cb user_command_parser_cb;

  /**
   * User-defined callback function that generates the device's status message.
   * Should return a dynamically allocated string or NULL if the device has
   * no status to report.
   */
  link_status_message_cb user_status_msg_cb;

  /**
   * User-defined callback function that generates the device's data message.
   * Should return a dynamically allocated string or NULL if the device has
   * no data to report.
   */
  link_data_message_cb user_data_msg_cb;

  /**
   * Private field for storing the pairing message.
   */
  char *_pair_msg;

} link_config_t;

/**
 * @brief Registers the device configuration, making it available for the
 * library's internal use.
 *
 * @param device Pointer to the device configuration structure.
 */
void link_register(link_config_t *device);

/**
 * @brief Starts all tasks related to ESP-NOW communication, including pairing
 * and message handling.
 *
 * @param force_pair If true, forces re-pairing even if a pairing exists.
 */
void link_start(bool force_pair);

/**
 * @brief Generates the status message based on the provided format string and
 * arguments.
 *
 * @param status_fmt The format string. If NULL, the format stored in
 * link_config_t is used.
 * @return A dynamically allocated string containing the formatted status
 * message.
 */
char *link_generate_status_message(const char *status_fmt, ...);

/**
 * @brief Retrieves the status message generated by user_status_msg_cb and sends
 * it via ESP-NOW.
 *
 * @return True if the message was sent successfully, false otherwise.
 */
bool link_send_status_msg();

/**
 * @brief Generates the data message based on the provided format string and
 * arguments.
 *
 * @param data_fmt The format string. If NULL, the format stored in
 * link_config_t is used.
 * @return A dynamically allocated string containing the formatted data message.
 */
char *link_generate_data_message(const char *data_fmt, ...);

/**
 * @brief Retrieves the data message generated by user_data_msg_cb and sends it
 * via ESP-NOW.
 *
 * @return True if the message was sent successfully, false otherwise.
 */
bool link_send_data_msg();

/**
 * @brief Returns the pairing message, generating it if necessary.
 *
 * @return The pairing message string.
 */
char *link_get_pair_msg();

#endif // LINK_CONFIG_H_
