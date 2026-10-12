/* SPDX-License-Identifier: GPL-3.0-only */
/* Match upstream Melodee's two-half, 85% overload policy. Include nested
 * scanner/USB interrupt time in the deadline, since DMA does not wait for it. */
#pragma once
static uint8_t smk37_shed_over, smk37_shed_req;
static void smk37_audio_budget_begin(void)
{
    if (smk37_shed_req) {
        smk37_shed_req = 0;
        shed_voice();
    }
}
static void smk37_audio_budget_end(uint32_t ticks)
{
    const uint32_t threshold = (HALF_FRAMES * 1000000u / FS) * 85u / 100u * FM1_TICKS_PER_US;
    smk37_shed_over = (uint8_t)(smk37_shed_over << 1 | (ticks > threshold));
    if ((smk37_shed_over & 3u) == 3u) smk37_shed_req = 1;
}
