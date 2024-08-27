#ifndef PAIR_H_
#define PAIR_H_

#include "esp_now_communication.h"

void enp_init(bool force_pair);
void enp_block_until_find_pair();
bool enp_get_gateway_mac(enc_mac_t* gateway_mac_out);
void enp_check_received_pairing_acceptance(enc_event_receive_cb_t *data);

#endif //PAIR_H_