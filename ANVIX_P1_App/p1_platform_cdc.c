/* v0.7.2: VCP completion-aware adapter. IMPORTANT: VCP_SendData has
 * no return code in this ST stack: if it internally refuses/discards TX,
 * the completion callback may never fire. That condition is diagnosed as a
 * stall; audit ST middleware vcp.c if seen. */
#include <stdint.h>
#include "vcp.h"
static volatile uint8_t tx_busy;
static volatile uint8_t tx_finished;
static uint8_t tx_buffer[64];
static volatile uint32_t tx_requests, tx_callbacks, tx_busy_rejects;
static void p1_tx_done(void) {
    tx_finished=1u;
    tx_callbacks++;
}
int P1_Platform_CDC_Tx(const uint8_t *data, uint16_t len)
{
    uint16_t i;
    if (!data || !len || len>sizeof(tx_buffer)) return -2;
    if (tx_busy) {tx_busy_rejects++; return -1;}
    tx_busy=1u;
    tx_finished=0u;
    for (i=0u; i<len; i++) tx_buffer[i]=data[i];
    tx_requests++;
    VCP_SendData(tx_buffer,len,p1_tx_done);
    return 0;  /* submission, NOT completion */
}
uint8_t P1_Platform_CDC_PollDone(void)
{
    if (!tx_busy || !tx_finished) return 0u;
    tx_finished=0u;
    tx_busy=0u;
    return 1u;
}
void P1_Platform_CDC_Stats(uint32_t *requests, uint32_t *callbacks,
                            uint32_t *rejections)
{
    if (requests) *requests=tx_requests;
    if (callbacks) *callbacks=tx_callbacks;
    if (rejections) *rejections=tx_busy_rejects;
}
