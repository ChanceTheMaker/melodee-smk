/* SPDX-License-Identifier: GPL-3.0-only */
/* One timer owns MIDI, audio SIE traffic and TRS. No main-loop audio poll. */
void smk37_usb_irq(void)
{
    fm1_timer5_ack();
    usb_poll();
#if MELODEE_USB_AUDIO
    ua_service();
#endif
#if SMK37_ENABLE_TRS
    smk37_trs_poll();
#endif
}
