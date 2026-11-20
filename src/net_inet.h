#ifndef VONORIT_NET_INET_H
#define VONORIT_NET_INET_H
#include <stdint.h>
int net_dns_resolve(const char* domain, uint8_t out_ip[4]);
int net_http_get(const char* host, char* out_buf, uint16_t max_len);
int net_udp_chat(const char* host, uint16_t port, const char* msg, char* out_reply, uint16_t max_len);
#endif
