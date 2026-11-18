#ifndef VONORIT_NET_H
#define VONORIT_NET_H

#include <stdint.h>

void net_init(void);
uint8_t net_is_present(void);
const char* net_driver_name(void);
const uint8_t* net_get_mac(void);
uint8_t net_link_status(void);

#endif
