/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../firmware/hal/smk37_analog_controls.h"
int main(void){
 for(unsigned c=0;c<6;c++)for(unsigned a=0;a<1024;a++)for(unsigned b=0;b<1024;b++){
  int d=(int)a-(int)b,t=c<4?8:3;
  int expected=(c<4&&a!=b&&(a==0||a==1023))||d>=t||d<=-t;
  assert(smk37_analog_moved(c,a,b)==expected);
 }
 smk37_fader_filter_t filter={0};
 assert(smk37_fader_filter(&filter,554)==554);
 unsigned anchor=554,changes=0;
 const unsigned noisy[]={540,558,563,550,547,561,552,555};
 for(unsigned i=0;i<200;i++){
  unsigned value=smk37_fader_filter(&filter,noisy[i%8]);
  if(smk37_analog_moved(0,value,anchor)){anchor=value;changes++;}
 }
 assert(changes==0);
 for(unsigned raw=554;raw<=650;raw++){
  unsigned value=smk37_fader_filter(&filter,raw);
  if(smk37_analog_moved(0,value,anchor)){anchor=value;changes++;}
 }
 assert(changes>8&&anchor>630);
 for(unsigned i=0;i<40;i++)smk37_fader_filter(&filter,1023);
 assert(smk37_fader_filter(&filter,1023)==1023);
 for(unsigned i=0;i<40;i++)smk37_fader_filter(&filter,0);
 assert(smk37_fader_filter(&filter,0)==0);

 for(unsigned control=0;control<4;control++){
  unsigned accepted=554;
  const unsigned jitter[]={551,555,553,557,554,552};
  for(unsigned i=0;i<sizeof jitter/sizeof jitter[0];i++)assert(!smk37_analog_moved(control,jitter[i],accepted));
  for(unsigned raw=555;raw<562;raw++)assert(!smk37_analog_moved(control,raw,accepted));
  assert(smk37_analog_moved(control,562,accepted));
  assert(smk37_analog_moved(control,546,accepted));
  assert(smk37_analog_moved(control,0,7));assert(smk37_analog_moved(control,1023,1016));
  assert(!smk37_analog_moved(control,0,0));assert(!smk37_analog_moved(control,1023,1023));
  assert(smk37_analog_moved(control,0,accepted));assert(smk37_analog_moved(control,1023,accepted));
 }
 for(unsigned control=4;control<6;control++){
  assert(!smk37_analog_moved(control,530,528));
  assert(smk37_analog_moved(control,531,528));assert(smk37_analog_moved(control,525,528));
 }
 puts("SMK fader: stationary jitter ignored; slow movement accumulates; endpoints and wheel sensitivity retained");
}
