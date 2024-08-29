#include "esp_now_communication.h"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "esp_now_pair.h"
#include "link.h"

static const char *TAG = "ENC";

#define ESPNOW_MAXDELAY 100

#define IS_BROADCAST_ADDR(addr)                                                \
  (memcmp(addr, esp_now_broadcast_mac.bytes, ESP_NOW_ETH_ALEN) == 0)
const enc_mac_t esp_now_broadcast_mac = {
    .bytes = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

static void on_esp_now_data_send(const uint8_t *mac_addr,
                                 esp_now_send_status_t status);
static void on_esp_now_data_receive(const esp_now_recv_info_t *esp_now_info,
                                    const uint8_t *data, int data_len);
static void esp_now_send_task(void *params);
static void esp_now_receive_task(void *params);

extern void link_message_parse(const char *data);

typedef struct {
  uint8_t mac_addr[ESP_NOW_ETH_ALEN];
  esp_now_send_status_t status;
} enc_event_send_cb_t;

typedef struct {
  char data[ESP_NOW_MAX_DATA_LEN];
  enc_mac_t dest_mac;

  // private
  QueueHandle_t *_ack_queue;
} enc_send_t;

static QueueHandle_t send_queue;
static QueueHandle_t receive_queue;
static QueueHandle_t send_result_queue;

void enc_init() {
  // init wifi module
  esp_netif_init();
  esp_event_loop_create_default();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_start();
  esp_wifi_set_channel(ENC_CHANNEL, WIFI_SECOND_CHAN_NONE);

  // Print MAC
  uint8_t mac[ESP_NOW_ETH_ALEN];
  esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
  ESP_LOGI(TAG, "ESPNOW MAC: " MACSTR, MAC2STR(mac));

  // init queue
  send_queue = xQueueCreate(ENC_SEND_QUEUE_SIZE, sizeof(enc_send_t));
  receive_queue =
      xQueueCreate(ENC_RECIEVE_QUEUE_SIZE, sizeof(enc_event_receive_cb_t));
  send_result_queue =
      xQueueCreate(ENC_RESULT_QUEUE_SIZE, sizeof(enc_event_send_cb_t));

  // esp now init
  if (esp_now_init() != ESP_OK) {
    ESP_LOGE(TAG, "esp_now_init ERROR");
  }

  // register esp now callbacks
  esp_now_register_send_cb(on_esp_now_data_send);
  esp_now_register_recv_cb(on_esp_now_data_receive);

  // create send task
  xTaskCreate(esp_now_send_task, "enc_send_task", 4096, NULL, 5, NULL);
  xTaskCreate(esp_now_receive_task, "enc_receive_task", 4096, NULL, 5, NULL);
}

static void on_esp_now_data_send(const uint8_t *mac_addr,
                                 esp_now_send_status_t status) {
  enc_event_send_cb_t send_cb;
  memcpy(send_cb.mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
  send_cb.status = status;

  if (xQueueSend(send_result_queue, &send_cb, ESPNOW_MAXDELAY) != pdTRUE) {
    ESP_LOGW(TAG, "Send send queue fail");
  }
}

/**
 * @brief Callback function of receiving ESPNOW data.
 */
static void on_esp_now_data_receive(const esp_now_recv_info_t *esp_now_info,
                                    const uint8_t *data, int data_len) {
  uint8_t *mac_addr = esp_now_info->src_addr;
  // uint8_t *des_addr = esp_now_info->des_addr;

  if (mac_addr == NULL || data == NULL || data_len <= 0) {
    ESP_LOGE(TAG, "Receive cb arg error");
    return;
  }

  enc_event_receive_cb_t cb;
  memset(&cb, 0, sizeof(enc_event_receive_cb_t));
  memcpy(&cb.esp_now_info, esp_now_info, sizeof(esp_now_recv_info_t));
  memcpy(&cb.data, data, data_len);
  cb.data_len = data_len;

  if (xQueueSend(receive_queue, &cb, ESPNOW_MAXDELAY) != pdTRUE) {
    ESP_LOGW(TAG, "Send receive queue fail");
  }
}

void esp_now_send_task(void *params) {
  static const char *TAG = "espnow_send_task";
  BaseType_t queue_status;
  enc_send_t data;
  esp_now_peer_info_t peer_info;
  esp_err_t err = ESP_OK;
  enc_event_send_cb_t result;

  while (1) {
    // wait for data
    queue_status = xQueueReceive(send_queue, &data, portMAX_DELAY);
    if (queue_status != pdPASS)
      continue;

    // create peer
    memset(&peer_info, 0, sizeof(esp_now_peer_info_t));
    peer_info.channel = ENC_CHANNEL;
    peer_info.encrypt = false;
    memcpy(peer_info.peer_addr, data.dest_mac.bytes, ESP_NOW_ETH_ALEN);
    err = esp_now_add_peer(&peer_info);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Error while adding new peer!- %s", esp_err_to_name(err));
      esp_now_del_peer(peer_info.peer_addr);
      continue;
    }

    // send data
    ESP_LOGI(TAG, "Sending data to " MACSTR ", message=%s",
             MAC2STR(peer_info.peer_addr), data.data);

    err = esp_now_send(peer_info.peer_addr, (uint8_t *)&(data.data),
                       strlen(data.data));
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Error while sending! - %s", esp_err_to_name(err));
      esp_now_del_peer(peer_info.peer_addr);
      continue;
    }

    // wait for the data to be sent
    queue_status = xQueueReceive(send_result_queue, &result, portMAX_DELAY);
    if (result.status == ESP_NOW_SEND_FAIL) {
      ESP_LOGW(TAG, "Sent data to " MACSTR ", with status %d (not received)",
               MAC2STR(result.mac_addr), result.status);
    } else {
      ESP_LOGI(TAG, "Sent data to " MACSTR ", with status %d (received)",
               MAC2STR(result.mac_addr), result.status);
    }

    ESP_LOGI(TAG, "_ack_queue = %p", data._ack_queue);

    if (data._ack_queue != NULL)
      xQueueSend(*data._ack_queue, &result.status, 0);

    // remove peer
    esp_now_del_peer(peer_info.peer_addr);
  }
}

bool check_mac(uint8_t *mac) {
  enc_mac_t gateway_mac;
  bool is_paired = enp_get_gateway_mac(&gateway_mac);
  if (!is_paired)
    return false;

  bool is_gateway_mac = (memcmp(gateway_mac.bytes, mac, ESP_NOW_ETH_ALEN) == 0);

  return (is_paired && is_gateway_mac);
}

void esp_now_receive_task(void *params) {
  static const char *TAG = "espnow_receive_task";
  BaseType_t queue_status;
  enc_event_receive_cb_t data;

  while (1) {
    // wait for data
    queue_status = xQueueReceive(receive_queue, &data, portMAX_DELAY);
    if (queue_status != pdPASS)
      continue;

    // Parsing data
    ESP_LOGI(TAG, "Recieved message \"%s\" from " MACSTR, data.data,
             MAC2STR(data.esp_now_info.src_addr));

    if (IS_BROADCAST_ADDR(data.esp_now_info.src_addr)) {
      ESP_LOGI(TAG, "Receive broadcast ESPNOW data");
    } else {
      enp_check_received_pairing_acceptance(&data);
      
      if (check_mac(data.esp_now_info.src_addr))
        link_message_parse(data.data);
    }
  }
}

bool enc_send_with_result(const char *data) {
  enc_send_t send_data = {0};
  strcpy(send_data.data, data);
  // send_data.dest_mac = enp_get_gateway_mac();
  bool is_paired = enp_get_gateway_mac(&send_data.dest_mac);
  if (!is_paired) {
    ESP_LOGW(TAG, "Device is not paired! Ommiting sending message");
    return false;
  }

  QueueHandle_t result = xQueueCreate(2, sizeof(esp_now_send_status_t));
  send_data._ack_queue = &result;
  xQueueSend(send_queue, &send_data, portMAX_DELAY);

  esp_now_send_status_t res;
  xQueueReceive(result, &res, portMAX_DELAY);
  ESP_LOGI(TAG, "data sent - status: %d", res);
  if (res == ESP_NOW_SEND_SUCCESS)
    return true;
  return false;
}

void enc_send_no_result(const char *data) {
  enc_send_t send_data = {0};
  memset(&send_data, 0, sizeof(enc_send_t));
  strcpy(send_data.data, data);
  // send_data.dest_mac = enp_get_gateway_mac();
  bool is_paired = enp_get_gateway_mac(&send_data.dest_mac);
  if (!is_paired) {
    ESP_LOGW(TAG, "Device is not paired! Ommiting sending message");
    return;
  }
  send_data._ack_queue = NULL;
  xQueueSend(send_queue, &send_data, portMAX_DELAY);
}

void enc_send_to_broadcast(const char *data) {
  enc_send_t send_data = {0};
  strcpy(send_data.data, data);
  memcpy(send_data.dest_mac.bytes, esp_now_broadcast_mac.bytes,
         ESP_NOW_ETH_ALEN);
  send_data._ack_queue = NULL;
  xQueueSend(send_queue, &send_data, portMAX_DELAY);
}