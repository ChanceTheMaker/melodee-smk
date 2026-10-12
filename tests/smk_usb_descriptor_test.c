/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static const uint8_t original[]={
#include "../firmware/src/usb_audio_desc.h"
};
#define MELODEE_SMK_HARDWARE 1
static const uint8_t smk[]={
#include "../firmware/src/usb_audio_desc.h"
};
int main(void){
 assert(sizeof original==sizeof smk);
 unsigned changes=0,terminal=0,formats=0;
 for(unsigned pos=0;pos<sizeof smk;){
  unsigned n=smk[pos];assert(n>=2&&pos+n<=sizeof smk);
  if(n==12&&smk[pos+1]==0x24&&smk[pos+2]==2&&smk[pos+3]==3){
   assert(smk[pos+4]==3&&smk[pos+5]==6);
   assert(original[pos+4]==0x13&&original[pos+5]==7);
   assert(smk[pos+7]==4&&smk[pos+8]==0&&smk[pos+9]==0);terminal++;
  }
  if(n==11&&smk[pos+1]==0x24&&smk[pos+2]==2&&smk[pos+4]==4){
   assert((smk[pos+5]==2&&smk[pos+6]==16)||(smk[pos+5]==3&&smk[pos+6]==24));
   assert(smk[pos+7]==1&&smk[pos+8]==0x44&&smk[pos+9]==0xac&&smk[pos+10]==0);formats++;
  }
  for(unsigned j=0;j<n;j++)if(original[pos+j]!=smk[pos+j]){
   assert(n==12&&smk[pos+1]==0x24&&smk[pos+2]==2&&smk[pos+3]==3&&(j==4||j==5));changes++;
  }
  pos+=n;
 }
 assert(changes==2&&terminal==1&&formats==2);
 puts("SMK capture: line terminal; four PCM16/24 tracks at 44.1 kHz; all other descriptor bytes unchanged");
}
