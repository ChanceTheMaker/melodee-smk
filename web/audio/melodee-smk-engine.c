/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
static uint16_t screen[240*240];
static void lcd_sync(void){}
static void lcd_blit(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t*p){if(x+w>240||y+h>240)return;for(unsigned j=0;j<h;j++)for(unsigned i=0;i<w;i++)screen[(y+j)*240+x+i]=p[j*w+i];}
static void lcd_fill(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint16_t c){c=(c>>8)|(c<<8);if(x+w>240||y+h>240)return;for(unsigned j=0;j<h;j++)for(unsigned i=0;i<w;i++)screen[(y+j)*240+x+i]=c;}
#include "../../firmware/src/melodee_smk_runtime.c"
static uint32_t ota_now_ms(void){return fm1_ms;}
static void drain_reply(void);
static void ota_idle(void){drain_reply();fm1_ms++;}
#define API __attribute__((visibility("default")))
static uint8_t reply[8192],request[2048];static unsigned reply_n;
static void drain_reply(void){while(so_r!=so_w){uint32_t p=sx_out_q[so_r++%SXQ],cin=p&15u,n=cin==4||cin==7?3:cin==6?2:1;for(unsigned i=0;i<n&&reply_n<sizeof reply;i++)reply[reply_n++]=(uint8_t)(p>>(8*(i+1)));}}
static int32_t output[CTL*2];static uint64_t frames;
API void synth_init(void){melodee_smk_init();usb.config=1;}
API void synth_midi(unsigned st,unsigned a,unsigned b){melodee_smk_midi(st,a,b);}
API int32_t*synth_render(void){fm1_ms=frames*1000/FS;mix_block(output,CTL);frames+=CTL;for(unsigned i=1;i<CTL;i+=2){unsigned w=scope_w++&(SCOPE_N-1);scope_buf[w]=output[i*2];}return output;}
API void fw_event(unsigned kind,unsigned id,int value){melodee_smk_event(kind,id,value);ui_input();}
API void fw_release(void){melodee_smk_release();}
API uint16_t*fw_screen(void){melodee_smk_tick();return screen;}
API uint32_t*fw_status(void){return melodee_smk_status();}
API uint8_t*fw_request(void){return request;}
API uint8_t*fw_reply(void){return reply;}
API unsigned fw_command(unsigned n){reply_n=0;if(n<4||n>sizeof request||request[0]!=125||request[1]!=70||request[2]!=76)return 0;ed_handle(request,n);drain_reply();return reply_n;}
API unsigned fw_changed(void){reply_n=0;ed_sync();drain_reply();return reply_n;}

#include "smk-session.c"
