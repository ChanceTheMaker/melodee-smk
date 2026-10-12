/* SPDX-License-Identifier: GPL-3.0-only */
/* Private namespace 7D MEL v1. Fixed storage region only; no arbitrary writes.
 * Provisioning requires a complete sequential raw backup, matching checksum,
 * same-unit fingerprint, stopped/quiet synth and explicit ERASE token. */
static int audio_quiet(void){
 if(transport_busy()||ua.play_alt)return 0;
 for(unsigned t=0;t<NTRK;t++)for(unsigned v=0;v<NVOICE;v++)if(trk[t].v[v].active)return 0;
 return 1;
}
static uint32_t ms_backup_pos,ms_backup_crc=~0u,ms_backup_at;
static uint32_t ms_u32(const uint8_t*p){uint32_t v=0;for(unsigned i=0;i<5;i++)v|=(uint32_t)p[i]<<(7*i);return v;}
static uint32_t ms_backup_addr(uint32_t pos){unsigned s=pos/4096;return (s<MS_SECTORS?ms_sector(s):MS_MARKER)+(pos%4096);}
static int melodee_storage_service(const uint8_t*f,uint32_t n){
 static const uint8_t prefix[]={125,'M','E','L',1};
 if(n<5||memcmp(f,prefix,5))return 0;if(n<6)return 1;unsigned op=f[5],rc=1;uint8_t data[256];uint32_t off=0,count=0;
 if(op==0&&n==6)rc=0;
 else if(op==1&&n==13&&f[10]<16){off=ms_u32(f+6);count=f[11]|(uint32_t)f[12]<<7;
  if(!ms_device_ok)rc=2;else if(song.playing||transport_req)rc=3;
  else if(!count||count>256||off>=96u*4096u||count>96u*4096u-off||off%4096+count>4096)rc=1;
  else if(ms_read(ms_backup_addr(off),data,count))rc=4;else{rc=0;if(!off){ms_backup_pos=0;ms_backup_crc=~0u;}if(off==ms_backup_pos){ms_backup_crc=ms_crc_step(ms_backup_crc,data,count);ms_backup_pos+=count;ms_backup_at=fm1_ms;}else ms_backup_pos=0;}
 }else if(op==2&&n==21&&f[10]<16&&f[15]<16&&!memcmp(f+16,"ERASE",5)){
  if(ms_setup_locked)rc=7;
  else if(!ms_device_ok||ms_u32(f+11)!=ms_unit)rc=2;
  else if(song.playing||transport_req||smk_keys||smk_pads||smk_sustain||!audio_quiet())rc=3;
  else if(ms_backup_pos!=96u*4096u||fm1_ms-ms_backup_at>1800000u||ms_u32(f+6)!=~ms_backup_crc)rc=5;
  else{uint32_t c=~0u;rc=0;for(unsigned p=0;p<96u*4096u;p+=256){if(ms_read(ms_backup_addr(p),data,256)){rc=4;break;}c=ms_crc_step(c,data,256);}if(!rc&&c!=ms_backup_crc)rc=5;
   if(!rc){
    /* Recheck after the checksum pass, then exclude the audio consumer before
     * interrupts resume. USB and scanner IRQs remain alive during formatting. */
    fm1_irq_off();
    if(smk_keys||smk_pads||smk_sustain||!audio_quiet())rc=3;
    else ms_setup_locked=1;
    fm1_irq_on();
   }
   if(!rc){ms_backup_pos=0;flash_ok=0;audio_silence();ms_formatting=1;rc=ms_format(ms_unit)?6:0;ms_formatting=0;ms_mounted=0;ui_message(rc?"STORAGE SETUP FAILED":"STORAGE READY: REBOOT");ui.force=1;}}
 }
 ed_n=0;ed_out[ed_n++]=0xf0;for(unsigned i=0;i<sizeof prefix;i++)ed_b(prefix[i]);ed_b(op);ed_b(rc);
 if(!rc&&op==0){ed_b(ms_device_ok);ed_b(ms_mounted);ed_bk_u32(ms_unit);ed_bk_u32(96u*4096u);}
 if(!rc&&op==1){ed_bk_u32(off);ed_b(count&127);ed_b(count>>7);ed_bk_u32(st_crc32(data,count));ed_bk_pack(data,count);}
 ed_send();return 1;
}
