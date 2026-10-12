/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define MELODEE_SMK_STORE_TEST 1
#include "../firmware/src/storage.c"
static uint8_t flash[0x100000],snapshot[0x100000],a[27904],b[27904],result[27904];
static int fail=-1,ops,partial;
static int writable(uint32_t a,uint32_t n){return n&&((a>=0x9d000&&a<0xc0000&&n<=0xc0000-a)||(a>=0xc2000&&a<0xff000&&n<=0xff000-a));}
static int ms_read(uint32_t a,void*p,uint32_t n){if(a>sizeof flash||n>sizeof flash-a)return -1;memcpy(p,flash+a,n);return 0;}
static int ms_erase(uint32_t a){assert(writable(a,4096)&&!(a&4095));if(fail>=0&&ops++==fail){if(partial)memset(flash+a,255,2048);return -1;}memset(flash+a,255,4096);return 0;}
static int ms_prog(uint32_t a,const void*p,uint32_t n){assert(writable(a,n)&&n<=256&&(a%256+n<=256));if(fail>=0&&ops++==fail){if(partial){const uint8_t*s=p;for(unsigned i=0;i<n/2;i++)flash[a+i]&=s[i];}return -1;}const uint8_t*s=p;for(unsigned i=0;i<n;i++){assert((flash[a+i]&s[i])==s[i]);flash[a+i]&=s[i];}return 0;}
int main(void){
 memset(flash,0x5a,sizeof flash);assert(!ms_mount(123));assert(st_save(OBJ_SETTINGS,a,10)<0);assert(!ms_format(123));assert(ms_mount(123));assert(!ms_mount(124));assert(ms_mount(123));
 for(unsigned i=0;i<sizeof a;i++){a[i]=(uint8_t)(i*31u);b[i]=(uint8_t)(i*17u+5u);}
 for(unsigned o=0;o<OBJ_COUNT;o++)if(!st_retired(o))assert(!st_save(o,a,o>=OBJ_BANK0&&o<OBJ_CZBANK0?sizeof a:ST_PAYLOAD_MAX));
 for(unsigned o=0;o<OBJ_COUNT;o++)if(!st_retired(o)){unsigned n=o>=OBJ_BANK0&&o<OBJ_CZBANK0?sizeof a:ST_PAYLOAD_MAX;assert(st_load(o,result,sizeof result)==(int)n);assert(!memcmp(a,result,n));}
 memcpy(snapshot,flash,sizeof flash);
 for(partial=0;partial<2;partial++)for(int cut=0;cut<140;cut++){memcpy(flash,snapshot,sizeof flash);assert(ms_mount(123));fail=cut;ops=0;int rc=st_save(OBJ_BANK0,b,sizeof b);fail=-1;assert(ms_mount(123));assert(st_load(OBJ_BANK0,result,sizeof result)==sizeof a);assert(!memcmp(result,rc?a:b,sizeof a));}
 partial=0;memcpy(flash,snapshot,sizeof flash);assert(ms_mount(123));assert(!st_save(OBJ_BANK0,b,sizeof b));st_hdr_t h;int c=st_current(OBJ_BANK0,&h);assert(c>=0);flash[ms_sector(c)+256]^=1;assert(st_load(OBJ_BANK0,result,sizeof result)==sizeof a);assert(!memcmp(result,a,sizeof a));
 for(unsigned i=0;i<0x9d000;i++)assert(flash[i]==0x5a);for(unsigned i=0xc0000;i<0xc2000;i++)assert(flash[i]==0x5a);for(unsigned i=0xff000;i<0x100000;i++)assert(flash[i]==0x5a);
 for(unsigned s=0;s<MS_SECTORS;s++)for(unsigned i=3840;i<4096;i++)assert(flash[ms_sector(s)+i]==255);
 fail=12;ops=0;assert(ms_format(123)<0);fail=-1;assert(!ms_mount(123));assert(!ms_format(123));
 puts("SMK storage: full inventory, torn writes, CRC fallback, ownership and protected ranges passed");return 0;
}
