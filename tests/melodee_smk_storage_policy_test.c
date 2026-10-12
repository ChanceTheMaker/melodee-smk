/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
static unsigned ms_device_ok,ms_mounted,ms_formatting;
#include "../firmware/hal/melodee_smk_storage_policy.h"
int main(void){
 for(unsigned a=0;a<0x100000;a++){
  int allowed=(a>=0x9d000&&a<0xc0000)||(a>=0xc2000&&a<0xff000);
  assert(!!MS_RANGE(a,1)==allowed);assert(!MS_RANGE(a,0));
 }
 assert(MS_RANGE(0x9d000,0x23000));assert(!MS_RANGE(0x9d000,0x23001));
 assert(MS_RANGE(0xc2000,0x3d000));assert(!MS_RANGE(0xc2000,0x3d001));
 assert(!MS_RANGE(0xbffff,2));assert(!MS_RANGE(0xc0000,1));
 assert(!MS_RANGE(0xc1fff,1));assert(!MS_RANGE(0xff000,1));
 assert(!MS_RANGE(0xffffffff,2));assert(!MS_RANGE(0x9d000,0xffffffff));
 for(unsigned d=0;d<2;d++)for(unsigned m=0;m<2;m++)for(unsigned f=0;f<2;f++){
  ms_device_ok=d;ms_mounted=m;ms_formatting=f;
  assert(!!FL_RANGE_OK(0x9d000,4096)==!!(d&&(m||f)));
  assert(!FL_RANGE_OK(0xc0000,4096));assert(!FL_RANGE_OK(0,4096));
 }
 puts("SMK production flash policy: ranges, overflow, ownership and device guards passed");
}
