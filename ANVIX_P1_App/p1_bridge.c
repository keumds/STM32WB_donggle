#include "p1_bridge.h"
#include <string.h>
/* Transparent FE43 notification payload -> USB FIFO.
   NO Cyton frame search, frame validation, byte filtering or transformation.
   The caller must serialize ring access; BLE STM32 sequencer and main loop
   normally run in the same cooperative application context. */
static uint8_t fifo[P1_USB_FIFO_SIZE];
static uint16_t rd, wr, count;
static p1_bridge_stats_t stats;

void p1_bridge_init(void)
{
  rd=wr=count=0u;
  memset(&stats,0,sizeof(stats));
}
void p1_bridge_reset_stream(void)
{
  /* No packet-parser state in raw passthrough mode. */
}
void p1_bridge_mark_disconnect(void)
{
  stats.disconnects++;
  /* Preserve already received bytes to drain via USB, including trailing
     partial packets. They are bytes, not parsed frames. */
}
void p1_bridge_ble_stream(const uint8_t *src,uint16_t len)
{
  uint16_t i;
  if(src==0 || len==0u) return;
  stats.notifications_rx++;
  stats.ble_bytes+=(uint32_t)len;
  if((uint32_t)count+(uint32_t)len > (uint32_t)P1_USB_FIFO_SIZE)
  {
    /* Preserve FIFO consistency: drop this COMPLETE notification on overflow. */
    stats.usb_fifo_overflow++;
    stats.dropped_bytes+=(uint32_t)len;
    return;
  }
  for(i=0u;i<len;i++)
  {
    fifo[wr]=src[i];
    wr=(uint16_t)((wr+1u)%P1_USB_FIFO_SIZE);
  }
  count=(uint16_t)(count+len);
  stats.fifo_used=count;
  if(count>stats.fifo_peak) stats.fifo_peak=count;
}
uint16_t p1_bridge_usb_peek(uint8_t *dst,uint16_t cap)
{
  uint16_t i,n=count;
  if(dst==0 || cap==0u) return 0u;
  if(n>cap) n=cap;
  for(i=0u;i<n;i++) dst[i]=fifo[(rd+i)%P1_USB_FIFO_SIZE];
  return n;
}
void p1_bridge_usb_commit(uint16_t len)
{
  if(len>count)len=count;
  rd=(uint16_t)((rd+len)%P1_USB_FIFO_SIZE);
  count=(uint16_t)(count-len);
  stats.fifo_used=count;
  stats.usb_bytes+=(uint32_t)len;
}
void p1_bridge_stats(p1_bridge_stats_t *out)
{
  if(out!=0) *out=stats;
}
