/* SPDX-License-Identifier: GPL-3.0-only */
#include "smk37_public_profile.h"
static uint8_t ms_device_ok,ms_formatting;
#include "melodee_smk_storage_policy.h"
static void audio_silence(void);
#include "storage.c"
#include "fm1_flash.h"
#include "smk37_flash_profile.h"
static uint8_t flash_ok;
static int ms_read(uint32_t a,void*dst,uint32_t n){
 if(a>0x100000u||n>0x100000u-a)return -1;uint8_t*p=dst;
 while(n){unsigned k=n>256?256:n;uint32_t f=irq_save();int rc=FL_FAR(fl_read_ram)(a,p,k);irq_restore(f);if(rc)return rc;a+=k;p+=k;n-=k;fm1_wdt_feed();}return 0;
}
static int ms_erase(uint32_t a){if(!ms_device_ok||(!ms_mounted&&!ms_formatting)||song.playing||transport_req||!MS_RANGE(a,4096))return -1;uint32_t took;audio_silence();fm1_wdt_feed();return fl_erase4k(a,&took);}
static int ms_prog(uint32_t a,const void*p,uint32_t n){if(!ms_device_ok||(!ms_mounted&&!ms_formatting)||song.playing||transport_req||!MS_RANGE(a,n))return -1;return fl_write(a,p,n);}
static int melodee_storage_probe(void){
 uint8_t trim[556],block[256];uint32_t f=irq_save(),id=FL_FAR(fl_jedec_ram)();irq_restore(f);ms_device_ok=ms_mounted=0;
 if(!smk37_flash_supported_id(id)||ms_read(0xc1000u,trim,sizeof trim)||!smk37_trim_keys_valid(trim,sizeof trim))return 0;
 uint32_t crc=~0u;for(unsigned a=0;a<0x4000;a+=sizeof block){if(ms_read(a,block,sizeof block))return 0;crc=ms_crc_step(crc,block,sizeof block);}if(~crc!=SMK37_PUBLIC_HEAD_CRC)return 0;
 ms_device_ok=1;ms_unit=st_crc32(trim,sizeof trim);return ms_mount(ms_unit);
}
