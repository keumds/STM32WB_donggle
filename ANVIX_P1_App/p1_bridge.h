#ifndef P1_BRIDGE_H
#define P1_BRIDGE_H
#include <stdint.h>
#define P1_USB_FIFO_SIZE 16384u
typedef struct {
  uint32_t packets_ok, bad_footer, lost_sequence, usb_fifo_overflow;
  uint32_t usb_bytes, ble_bytes, notifications_rx, resync_count, disconnects;
  uint32_t dropped_bytes;
  uint16_t fifo_used;
  uint16_t fifo_peak;
} p1_bridge_stats_t;
void p1_bridge_init(void);
void p1_bridge_ble_stream(const uint8_t *src,uint16_t len);
uint16_t p1_bridge_usb_peek(uint8_t *dst,uint16_t maxlen);
void p1_bridge_usb_commit(uint16_t len);
void p1_bridge_stats(p1_bridge_stats_t *out);
void p1_bridge_reset_stream(void);
void p1_bridge_mark_disconnect(void);
#endif
