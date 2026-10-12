/* SPDX-License-Identifier: GPL-3.0-only */
static void melodee_usb_service(void){
    static const uint8_t query[]={0,0x32,0x45,0,0,0,0x40,0x7f};
    const uint8_t *p;uint32_t n;
    if(!ota_frame_get(&p,&n)){if(!ms_setup_locked)ed_service();return;}
    if(n==sizeof query&&!memcmp(p,query,n)){
        ota_frame_done();ota_wire_send(smk37_update_identity_wire,sizeof smk37_update_identity_wire);return;
    }
#if FELUCCA_FLASH
    if(melodee_storage_service(p,n)){ota_frame_done();return;}
#endif
    if(ms_setup_locked){ota_frame_done();return;}
    /* Classify the pending frame before editor processing. Never clear the
     * queue afterward: an IRQ may have delivered the next request meanwhile. */
    if(n<3 || p[0]!=125 || p[1]!='F' || p[2]!='L'){
        ota_frame_done();return;
    }
    ed_service();
}
