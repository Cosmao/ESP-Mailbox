#ifndef _wifi_h
#define _wifi_h

#include <stdint.h>

uint8_t wifi_init_station(void);
extern uint8_t dont_reconnect;

#endif // !_wifi_h
