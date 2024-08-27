#ifndef DEVICE_CONFIG_H_
#define DEVICE_CONFIG_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#define PAIR_MSG_FMT "SHPR:{\"type\":%d,\"cfg\":%s}"
#define PAIR_ACCEPT "SHPR:PAIRED"

#define DEVICE_CONFIG_SIZE 32
#define DEVICE_STATUS_FMT_SIZE 32
#define DEVICE_DATA_FMT_SIZE 32

#define DEVICE_MAX_COMMANDS 10
#define DEVICE_COMMAND_MAX_SIZE 32

typedef void (*device_command_cb)(const char *cmd);
typedef char *(*device_status_message_cb)(void);
typedef char *(*device_data_message_cb)(void);

typedef struct {
  // Typ urządzenia, numer, który będzie wysyłany w wiadomości parowania
  int type;

  // wiadomośc config, która jest wysyłana przy parowaniu, w formacje JSON np.:
  // {"data_interval": 300}
  // jeżeli urządzenie nie uzywa to zostawić puste
  char config[DEVICE_CONFIG_SIZE];

  // Format statusu urządzenia
  char status_fmt[DEVICE_STATUS_FMT_SIZE];

  // Format danych urządzenia
  char data_fmt[DEVICE_DATA_FMT_SIZE];

  // Tablica komend, na które reaguje
  // urządzenie, jeżeli w komendzie
  // występują zmienne to wpisz * np.:
  // SET_BRIGHTNESS=*
  char commands[DEVICE_MAX_COMMANDS][DEVICE_COMMAND_MAX_SIZE];

  // funkcja użytkownika, wykonuje się gdy urządzenie otrzyma komende, zapisaną
  // w tablicy commands
  device_command_cb user_command_parser_cb;

  // Funkcja którą musi utworzyć użytkownik
  // zwraca ona napis statusu urzadzenia
  // jeżeli urządzenie ma nie zwracać statusu
  // to niech ta funkcja zwraca NULL
  /*
    przykład

    char *on_status_message(void) {
      char *status;
      if (lamp_is_on())
        status = device_generate_status_message(NULL, "ON");
      else
        status = device_generate_status_message(NULL, "OFF");
      return status;
    }
  */
  device_status_message_cb user_status_msg_cb;

  // funkcja użytkownika, która zwraca napis data
  // jeżeli urządzenie ma nie zwracać danych
  // to niech ta funkcja zwraca NULL
  device_data_message_cb user_data_msg_cb;

  // private

  char *_pair_msg;

} device_config_t;

void device_register(device_config_t *device);

// uruchamia wszystkie zadania związane z esp_now
// parowanie, komuniakcja itp
void device_start(bool force_pair);

// status_fmt can be NULL, if is saved in device_config_t
char *device_generate_status_message(const char *status_fmt, ...);

// gets status message from user_status_msg_cb and send it
bool device_send_status_msg();

// data_fmt can be NULL, if is saved in device_config_t
char *device_generate_data_message(const char *data_fmt, ...);

// gets data message from user_data_msg_cb and send it
bool device_send_data_msg();

// zwraca wiadomość parowania
char *device_get_pair_msg();

#endif // DEVICE_CONFIG_H_