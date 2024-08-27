#ifndef ESP_NOW_COMMUNICATION_H_
#define ESP_NOW_COMMUNICATION_H_

#include "esp_now.h"
#include "freertos/FreeRTOS.h"

#define ENC_CHANNEL 1
#define ENC_SEND_QUEUE_SIZE 5
#define ENC_RECIEVE_QUEUE_SIZE 5
#define ENC_RESULT_QUEUE_SIZE 2

typedef union {
  uint8_t bytes[6];
  uint64_t value;
} enc_mac_t;

typedef struct {
  esp_now_recv_info_t esp_now_info;
  char data[ESP_NOW_MAX_DATA_LEN];
  int data_len;
} enc_event_receive_cb_t;

extern const enc_mac_t esp_now_broadcast_mac;

void enc_init();
bool enc_send_with_result(const char *data);
void enc_send_no_result(const char *data);
void enc_send_to_broadcast(const char *data);

#endif // ESP_NOW_COMMUNICATION_H_