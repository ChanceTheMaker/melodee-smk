/* SPDX-License-Identifier: GPL-3.0-only */
/* Real voice selection with the SMK deadline scheduler; no device access. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
#define FM1_TICKS_PER_US 24u
static uint32_t shed_count;
#include "../firmware/src/audio_shed_voice.h"
#include "../firmware/src/smk37_audio_budget.h"
int main(void)
{
    memset(trk,0,sizeof trk);
    trk[0].p[P_VOICE]=V_POLY;trk[1].p[P_VOICE]=V_MONO;
    voice_t *bass=&trk[0].v[0],*extra=&trk[0].v[1],*tail=&trk[0].v[2],*lead=&trk[1].v[0];
    bass->active=bass->gate=1;bass->note=48;
    extra->active=extra->gate=1;extra->note=60;
    tail->active=1;tail->note=64;tail->env=1000;
    lead->active=lead->gate=1;lead->note=72;
    uint32_t limit=(HALF_FRAMES*1000000u/FS)*85u/100u*FM1_TICKS_PER_US;
    smk37_audio_budget_end(limit+1);smk37_audio_budget_begin();assert(!shed_count);
    smk37_audio_budget_end(limit);smk37_audio_budget_begin();assert(!shed_count);
    smk37_audio_budget_end(limit+1);smk37_audio_budget_begin();assert(!shed_count);
    smk37_audio_budget_end(limit+1);assert(smk37_shed_req);
    smk37_audio_budget_begin();assert(shed_count==1&&tail->stage==4&&bass->gate&&lead->gate&&extra->gate);
    smk37_audio_budget_begin();assert(shed_count==1);
    smk37_audio_budget_end(limit+1);smk37_audio_budget_begin();
    assert(shed_count==2&&extra->stage==4&&bass->gate&&lead->gate);
    smk37_audio_budget_end(limit+1);smk37_audio_budget_begin();assert(shed_count==2);
    puts("SMK overload: transient tolerance, consecutive deadlines, tail-first fades and protected bass/lead passed");
    return 0;
}
