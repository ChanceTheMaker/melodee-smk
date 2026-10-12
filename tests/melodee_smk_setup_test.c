/* SPDX-License-Identifier: GPL-3.0-only */
#define main store_tests
#include "melodee_smk_storage_test.c"
#undef main
#define NTRK 4
#define NVOICE 5
static struct {struct {int active;}v[NVOICE];}trk[NTRK];
static struct {int play_alt;}ua;
static struct {int playing;}song;
static struct {int force;}ui;
static unsigned transport_req,smk_keys,smk_pads,smk_sustain,fm1_ms,ms_device_ok=1,flash_ok,ms_formatting;
static unsigned ms_setup_locked,irq_disabled,silenced;
static void fm1_irq_off(void){irq_disabled=1;}
static void fm1_irq_on(void){irq_disabled=0;}
static void audio_silence(void){assert(ms_setup_locked);silenced++;}
static uint8_t ed_out[2048];static unsigned ed_n;
static int transport_busy(void){return song.playing||transport_req;}
static void ui_message(const char*s){(void)s;}
static void ed_b(unsigned v){ed_out[ed_n++]=v&127;}
static void ed_bk_u32(uint32_t v){for(unsigned i=0;i<5;i++)ed_b(v>>(7*i));}
static void ed_bk_pack(const uint8_t*p,unsigned n){(void)p;(void)n;}
static void ed_send(void){}
#include "../firmware/src/melodee_smk_storage_service.c"
static void put32(uint8_t*p,uint32_t v){for(unsigned i=0;i<5;i++)p[i]=(v>>(7*i))&127;}
int main(void){
 memset(flash,0x5a,sizeof flash);ms_unit=123;uint8_t init[21]={125,'M','E','L',1,2};put32(init+11,123);memcpy(init+16,"ERASE",5);
 assert(melodee_storage_service(init,sizeof init));assert(ed_out[7]==5);assert(flash[MS_MARKER]==0x5a);
 uint8_t read[13]={125,'M','E','L',1,1};read[11]=0;read[12]=2;
 song.playing=1;assert(melodee_storage_service(read,13));assert(ed_out[7]==3);song.playing=0;
 put32(read+6,96u*4096u);assert(melodee_storage_service(read,13));assert(ed_out[7]==1);
 for(unsigned off=0;off<96u*4096u;off+=256){put32(read+6,off);assert(melodee_storage_service(read,13));assert(!ed_out[7]);}
 put32(init+6,~ms_backup_crc);ua.play_alt=1;melodee_storage_service(init,21);assert(ed_out[7]==3);ua.play_alt=0;
 trk[0].v[0].active=1;melodee_storage_service(init,21);assert(ed_out[7]==3);trk[0].v[0].active=0;
 init[6]^=1;melodee_storage_service(init,21);assert(ed_out[7]==5);init[6]^=1;
 flash[0x9d123]^=1;melodee_storage_service(init,21);assert(ed_out[7]==5);flash[0x9d123]^=1;
 melodee_storage_service(init,21);assert(!ed_out[7]);assert(!ms_mounted&&!flash_ok);assert(ms_setup_locked&&silenced==1&&!irq_disabled);assert(ms_mount(123));
 melodee_storage_service(init,21);assert(ed_out[7]==7);
 for(unsigned i=0;i<0x9d000;i++)assert(flash[i]==0x5a);for(unsigned i=0xc0000;i<0xc2000;i++)assert(flash[i]==0x5a);
 puts("SMK setup: backup, checksum, quiet-audio, bounds and explicit-init guards passed");return 0;
}
