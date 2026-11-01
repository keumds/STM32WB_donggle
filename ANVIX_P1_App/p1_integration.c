/* v0.7.2: remove FIFO bytes only AFTER VCP completion callback. */
#include "p1_integration.h"
#include "p1_bridge.h"
#include "p1_commands.h"
#include "app_conf.h"
#include "dbg_trace.h"
#include "stm32wbxx_hal.h"
#ifndef P1_DIAG_UART_ENABLE
#define P1_DIAG_UART_ENABLE 0  /* Change to 1 only when separate LPUART1 is wired */
#endif
extern int P1_Platform_CDC_Tx(const uint8_t *data,uint16_t len);
extern uint8_t P1_Platform_CDC_PollDone(void);
extern void P1_Platform_CDC_Stats(uint32_t *,uint32_t *,uint32_t *);
extern int P1_Platform_BLE_Write_FE41(const uint8_t *data,uint16_t len);
/* Live diagnostic globals for Keil Watch (no UART or USB text required). */
volatile uint32_t p1_diag_ble_bps, p1_diag_usb_bps, p1_diag_drop_bps;
volatile uint32_t p1_diag_notify_per_s, p1_diag_notify_total;
volatile uint32_t p1_diag_len_231, p1_diag_len_others, p1_diag_len_last;
volatile uint32_t p1_diag_notify_max_gap_ms, p1_diag_last_notify_ms;
volatile uint32_t p1_diag_fifo_used, p1_diag_fifo_peak, p1_diag_overflow;
volatile uint32_t p1_diag_usb_req, p1_diag_usb_cb, p1_diag_usb_busy, p1_diag_stall;
/* v0.7.4 transport high-water and estimate of FE43 notification throughput */
volatile uint32_t p1_diag_notify_231_per_s, p1_diag_notify_other_per_s;
static uint32_t diag_prev_231, diag_prev_other;
static uint32_t diag_prev_notify;
static uint8_t usb_tx[64];
static uint16_t pending_len;
static uint32_t request_tick;
static uint32_t last_diag_tick;
static uint32_t diag_prev_ble,diag_prev_usb,diag_prev_drop;
static uint32_t stall_count;
static uint8_t stall_reported;
void P1_Startup(void) {p1_bridge_init(); pending_len=0u; last_diag_tick=HAL_GetTick();}
void P1_Disconnected(void) {p1_bridge_mark_disconnect();}
void P1_Notify_FE43(const uint8_t *data,uint16_t len)
{
    uint32_t now=HAL_GetTick();
    if (p1_diag_last_notify_ms) {
        uint32_t gap=now-p1_diag_last_notify_ms;
        if (gap>p1_diag_notify_max_gap_ms) p1_diag_notify_max_gap_ms=gap;
    }
    p1_diag_last_notify_ms=now;
    p1_diag_len_last=len;
    if (len==231u) p1_diag_len_231++;
    else p1_diag_len_others++;
    p1_bridge_ble_stream(data,len);
}
void P1_Usb_Task(void)
{
    uint32_t now=HAL_GetTick();
    p1_bridge_stats_t st;
    uint32_t req,done,busy;
    if (pending_len) {
        if (P1_Platform_CDC_PollDone()) {
            p1_bridge_usb_commit(pending_len);
            pending_len=0u;
            stall_reported=0u;
        } else if (!stall_reported && ((uint32_t)(now-request_tick) > 1000u)) {
            stall_reported=1u;
            stall_count++;
#if P1_DIAG_UART_ENABLE
            APP_DBG_MSG("[P1 USB] TX stalled >1000ms: inspect VCP callback\r\n");
#endif
        }
    }
    if (!pending_len) {
        uint16_t n=p1_bridge_usb_peek(usb_tx,(uint16_t)sizeof(usb_tx));
        if (n && P1_Platform_CDC_Tx(usb_tx,n)==0) {
            pending_len=n;
            request_tick=now;
            /* May have completed synchronously. Commit on next task call. */
        }
    }
    if ((uint32_t)(now-last_diag_tick)>=1000u) {
        last_diag_tick=now;
        p1_bridge_stats(&st);
        P1_Platform_CDC_Stats(&req,&done,&busy);
        p1_diag_ble_bps=st.ble_bytes-diag_prev_ble;
        p1_diag_usb_bps=st.usb_bytes-diag_prev_usb;
        p1_diag_drop_bps=st.dropped_bytes-diag_prev_drop;
        p1_diag_notify_per_s=st.notifications_rx-diag_prev_notify;
        p1_diag_notify_total=st.notifications_rx;
        p1_diag_notify_231_per_s=p1_diag_len_231-diag_prev_231;
        p1_diag_notify_other_per_s=p1_diag_len_others-diag_prev_other;
        diag_prev_231=p1_diag_len_231;
        diag_prev_other=p1_diag_len_others;
        diag_prev_notify=st.notifications_rx;
        p1_diag_fifo_used=st.fifo_used;
        p1_diag_fifo_peak=st.fifo_peak;
        p1_diag_overflow=st.usb_fifo_overflow;
        p1_diag_usb_req=req;
        p1_diag_usb_cb=done;
        p1_diag_usb_busy=busy;
        p1_diag_stall=stall_count;
#if P1_DIAG_UART_ENABLE
        APP_DBG_MSG("[P1 1s] BLE=%luB/s USB_done=%luB/s DROP=%luB/s fifo=%u peak=%u overflow=%lu pending=%u vcp_req=%lu vcp_cb=%lu busy=%lu stall=%lu\r\n",
            (unsigned long)(st.ble_bytes-diag_prev_ble),
            (unsigned long)(st.usb_bytes-diag_prev_usb),
            (unsigned long)(st.dropped_bytes-diag_prev_drop),
            (unsigned)st.fifo_used,(unsigned)st.fifo_peak,
            (unsigned long)st.usb_fifo_overflow,(unsigned)pending_len,
            (unsigned long)req,(unsigned long)done,(unsigned long)busy,
            (unsigned long)stall_count);
#endif
        diag_prev_ble=st.ble_bytes;
        diag_prev_usb=st.usb_bytes;
        diag_prev_drop=st.dropped_bytes;
    }
}
void P1_Usb_Rx(const uint8_t *data,uint16_t len) {
    p1_commands_usb_rx(data,len,P1_Platform_BLE_Write_FE41);
}
