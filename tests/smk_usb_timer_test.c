/* SPDX-License-Identifier: GPL-3.0-only */
#define MELODEE_SMK_HARDWARE 1
#define main usb_driver_suite
#include "usb_audio_driver_test.c"
#undef main
#define SMK37_ENABLE_TRS 1
static unsigned timer_acks,trs_polls;
static void fm1_timer5_ack(void){timer_acks++;}
static void smk37_trs_poll(void){trs_polls++;}
#include "../firmware/src/melodee_smk_usb_irq.h"
int main(void){
 usb.up=usb.config=1;
 common[S_INTRRX1E]=2;
 ua_hw_stop();
 assert(ua_set_interface(5,1));
 unsigned initial=ua.tx_packets;
 for(unsigned i=1;i<=50;i++){
  regs[2][S_TXCSR1]=0; /* host consumed the previous capture packet */
  frame(i);
  smk37_usb_irq();
  assert(ua.tx_packets==initial+i);
  assert(regs[2][S_TXCSR1]&1);
 }
 assert(timer_acks==50&&trs_polls==50);
 assert(common[S_INTRRX1E]&2); /* MIDI still enabled */
 usb.up=0;unsigned sent=ua.tx_packets;
 smk37_usb_irq();assert(ua.tx_packets==sent);
 puts("SMK production USB timer: capture rearmed over 50 host frames; MIDI/TRS retained; detached bus untouched");
}
