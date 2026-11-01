#include "p1_commands.h"
/* Transparent command forwarding. Queue in integration layer if BLE GATT busy;
   must not silently drop chunks or issue overlapping writes. */
void p1_commands_usb_rx(const uint8_t *src,uint16_t len,p1_ble_write_t send){
 if(src && len && send) (void)send(src,len);
}
