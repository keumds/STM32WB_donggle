#ifndef P1_INTEGRATION_H
#define P1_INTEGRATION_H
#include <stdint.h>
/* Wire these into ST's BLE_p2pClient + USB CDC application. */
void P1_Notify_FE43(const uint8_t *data,uint16_t len);
void P1_Usb_Task(void);
void P1_Usb_Rx(const uint8_t *data,uint16_t len);
void P1_Startup(void);
void P1_Disconnected(void);
#endif
