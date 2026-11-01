#ifndef P1_COMMANDS_H
#define P1_COMMANDS_H
#include <stdint.h>
/* CDC RX chunks -> BLE FE41 writes; caller implements send(). */
typedef int (*p1_ble_write_t)(const uint8_t *data,uint16_t len);
void p1_commands_usb_rx(const uint8_t *src,uint16_t len,p1_ble_write_t send);
#endif
