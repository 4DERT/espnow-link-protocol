#include "esp_now_pair.h"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "link.h"
#include "esp_now_communication.h"

static const char *TAG = "enp";

#define NVS_MAC_KEY "gw_mac"
#define NVS_NAME "PAIR"

static SemaphoreHandle_t xMutex;

static enc_mac_t gateway;
static bool is_paired = false;
static bool is_pairing = false;

static nvs_handle_t nvs;
static TaskHandle_t pair_task_handle;

static inline void wait_random_time_and_send_status_and_data() {
  vTaskDelay(pdMS_TO_TICKS(esp_random() % 300));
  link_send_status_msg();
  link_send_data_msg();
}

void pair_task(void *params) {
  while (1) {
    ESP_LOGI(TAG, "Sending pair request");
    enc_send_to_broadcast(link_get_pair_msg());

    // wait some time for response
    int time = 4000 + (esp_random() % 2000);
    if (ulTaskNotifyTake(true, pdMS_TO_TICKS(time))) {
      ESP_LOGI(TAG, "Device " MACSTR " accepted pair", MAC2STR(gateway.bytes));

      xSemaphoreTake(xMutex, portMAX_DELAY);
      is_pairing = false;
      is_paired = true;
      xSemaphoreGive(xMutex);

      nvs_set_u64(nvs, NVS_MAC_KEY, gateway.value);
      nvs_commit(nvs);
      wait_random_time_and_send_status_and_data();
      pair_task_handle = NULL;
      vTaskDelete(NULL);
    }
  }
}

// Public

void enp_init(bool force_pair) {
  xMutex = xSemaphoreCreateMutex();

  xSemaphoreTake(xMutex, portMAX_DELAY);
  is_pairing = true;
  is_paired = false;
  xSemaphoreGive(xMutex);

  nvs_open(NVS_NAME, NVS_READWRITE, &nvs);

  if (force_pair) {
    ESP_LOGI(TAG, "force_pair");
    nvs_set_u64(nvs, NVS_MAC_KEY, 0ULL);
    nvs_commit(nvs);
  } else {
    nvs_get_u64(nvs, NVS_MAC_KEY, &gateway.value);

    if (gateway.value) {
      ESP_LOGI(TAG, "Got gateway MAC from NVS - " MACSTR,
               MAC2STR(gateway.bytes));
      xSemaphoreTake(xMutex, portMAX_DELAY);
      is_pairing = false;
      is_paired = true;
      xSemaphoreGive(xMutex);
    }
  }

  xSemaphoreTake(xMutex, portMAX_DELAY);
  bool _is_paired = is_paired;
  xSemaphoreGive(xMutex);

  if (!_is_paired) {
    ESP_LOGI(TAG, "Starting the pairing procedure");
    xTaskCreate(pair_task, "pair_task", 4096, NULL, 1, &pair_task_handle);
  } else {
    wait_random_time_and_send_status_and_data();
  }

  // parowanie
}

bool enp_get_gateway_mac(enc_mac_t *gateway_mac_out) {
  xSemaphoreTake(xMutex, portMAX_DELAY);
  bool _is_paired = is_paired;
  xSemaphoreGive(xMutex);

  if (_is_paired) {
    *gateway_mac_out = gateway;
    return true;
  }
  return false;
}

void enp_block_until_find_pair() {}

void enp_check_received_pairing_acceptance(enc_event_receive_cb_t *data) {
  xSemaphoreTake(xMutex, portMAX_DELAY);
  bool _is_pairing = is_pairing;
  xSemaphoreGive(xMutex);

  if (!strcmp(data->data, PAIR_ACCEPT) && pair_task_handle != NULL &&
      _is_pairing) {
    memcpy(gateway.bytes, data->esp_now_info.src_addr, ESP_NOW_ETH_ALEN);
    xTaskNotifyGive(pair_task_handle);
  }
}
