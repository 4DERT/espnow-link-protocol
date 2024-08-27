#ifndef LINK_CONFIG_H_
#define LINK_CONFIG_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#define PAIR_MSG_FMT "SHPR:{\"type\":%d,\"cfg\":%s}"
#define PAIR_ACCEPT "SHPR:PAIRED"

#define LINK_CONFIG_SIZE 32
#define LINK_STATUS_FMT_SIZE 32
#define LINK_DATA_FMT_SIZE 32

#define LINK_MAX_COMMANDS 10
#define LINK_COMMAND_MAX_SIZE 32

typedef void (*link_command_cb)(const char *cmd);
typedef char *(*link_status_message_cb)(void);
typedef char *(*link_data_message_cb)(void);

typedef struct {
  // Typ urządzenia, numer, który będzie wysyłany w wiadomości parowania
  int type;

  // wiadomośc config, która jest wysyłana przy parowaniu, w formacje JSON np.:
  // {"data_interval": 300}
  // jeżeli urządzenie nie uzywa to zostawić puste
  char config[LINK_CONFIG_SIZE];

  // Format statusu urządzenia
  char status_fmt[LINK_STATUS_FMT_SIZE];

  // Format danych urządzenia
  char data_fmt[LINK_DATA_FMT_SIZE];

  // Tablica komend, na które reaguje
  // urządzenie, jeżeli w komendzie
  // występują zmienne to wpisz * np.:
  // SET_BRIGHTNESS=*
  char commands[LINK_MAX_COMMANDS][LINK_COMMAND_MAX_SIZE];

  // funkcja użytkownika, wykonuje się gdy urządzenie otrzyma komende, zapisaną
  // w tablicy commands
  link_command_cb user_command_parser_cb;

  // Funkcja którą musi utworzyć użytkownik
  // zwraca ona napis statusu urzadzenia
  // jeżeli urządzenie ma nie zwracać statusu
  // to niech ta funkcja zwraca NULL
  link_status_message_cb user_status_msg_cb;

  // funkcja użytkownika, która zwraca napis data
  // jeżeli urządzenie ma nie zwracać danych
  // to niech ta funkcja zwraca NULL
  link_data_message_cb user_data_msg_cb;

  // private

  char *_pair_msg;

} link_config_t;

void link_register(link_config_t *device);

// uruchamia wszystkie zadania związane z esp_now
// parowanie, komuniakcja itp
void link_start(bool force_pair);

// status_fmt can be NULL, if is saved in link_config_t
char *link_generate_status_message(const char *status_fmt, ...);

// gets status message from user_status_msg_cb and send it
bool link_send_status_msg();

// data_fmt can be NULL, if is saved in link_config_t
char *link_generate_data_message(const char *data_fmt, ...);

// gets data message from user_data_msg_cb and send it
bool link_send_data_msg();

// zwraca wiadomość parowania
char *link_get_pair_msg();

#endif // LINK_CONFIG_H_
