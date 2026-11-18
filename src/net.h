#ifndef VONORIT_NET_H
#define VONORIT_NET_H
#include <stdint.h>
void net_init(void);
uint8_t net_is_present(void);
const char* net_driver_name(void);
const uint8_t* net_get_mac(void);
uint8_t net_link_status(void);
int net_ping_ipv4(const uint8_t ip[4],uint32_t* rtt);
void net_send_frame(const uint8_t* frame, uint16_t len);
uint16_t net_poll_frame(uint8_t* out, uint16_t max_len);
int net_resolve_arp(const uint8_t target_ip[4], uint8_t out_mac[6]);
#endif
