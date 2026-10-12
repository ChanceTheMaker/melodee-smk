/* SPDX-License-Identifier: GPL-3.0-only */
#include "smk37_midi_tx.h"
#include "smk37_uart_tx.h"
static smk37_midi_tx_t smk37_trs_queue;
static uint8_t smk37_trs_dma[16] __attribute__((aligned(16)));
static uint8_t smk37_trs_running,smk37_trs_busy,smk37_trs_stopping;
static uint8_t smk37_trs_dma_count;
static uint32_t smk37_trs_started,smk37_trs_faults;
/* Audio ISR is the only producer. USB remains available regardless of TRS. */
static void smk37_trs_output(uint32_t packet){if(smk37_trs_running&&!smk37_trs_stopping)(void)smk37_midi_tx_push(&smk37_trs_queue,packet);}
/* Main loop; the short setup is serialized against scanner/USB/audio. */
static void smk37_trs_configure(void){
    fm1_irq_off();
    if(smk37_trs_enabled&&!smk37_trs_running){memset(&smk37_trs_queue,0,sizeof smk37_trs_queue);smk37_trs_busy=smk37_trs_stopping=0;smk37_uart_tx_start();smk37_trs_running=1;}
    else if(!smk37_trs_enabled&&smk37_trs_running&&!smk37_trs_stopping){smk37_trs_stopping=1;smk37_trs_queue.recover=1;}
    fm1_irq_on();
}
/* Existing 2 kHz USB timer is lower priority than audio. No busy wait and no
 * DMA-buffer mutation until completion. Disabling drains a channel panic. */
static void smk37_trs_poll(void){
    if(!smk37_trs_running)return;
    if(smk37_trs_busy){
        if(!smk37_uart_tx_done()){
            if(fm1_ticks()-smk37_trs_started>24000000u){smk37_uart_tx_stop();smk37_trs_running=smk37_trs_enabled=0;++smk37_trs_faults;}
            return;
        }smk37_trs_bytes+=smk37_trs_dma_count;smk37_trs_busy=0;
    }
    unsigned n=0;while(n<sizeof smk37_trs_dma&&smk37_midi_tx_next(&smk37_trs_queue,&smk37_trs_dma[n]))++n;
    if(n){smk37_trs_started=fm1_ticks();smk37_trs_busy=1;smk37_trs_dma_count=n;smk37_uart_tx_send(smk37_trs_dma,n);}
    else if(smk37_trs_stopping&&!smk37_trs_queue.recover){smk37_uart_tx_stop();smk37_trs_running=0;}
}
