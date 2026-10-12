/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
/* Owner-verified physical order, zero-based ADC indices. */
static const uint8_t smk37_pad_input[16] = {
    3,0,1,2,4,6,7,5,11,8,9,10,12,14,15,13
};
static const uint8_t smk37_fader_input[4] = {21,18,19,20};
/* Update only on a fresh ADC conversion, never repeatedly on a stale sample.
 * Quarter-rate integer low-pass retains fractional history and converges to
 * both exact endpoints; a stationary fader is not initialized from zero. */
/* Store sum+1 so zero denotes an uninitialized filter without another flag. */
typedef struct { uint32_t biased_sum; } smk37_fader_filter_t;
static inline unsigned smk37_fader_filter(smk37_fader_filter_t *s, unsigned raw)
{
    unsigned sum=s->biased_sum;
    if(!sum)sum=(raw<<2)+1;
    else sum+=raw-((sum-1)>>2);
    s->biased_sum=sum;
    return (sum-1)>>2;
}
/* A fader's 10-bit ADC needs about eight counts per 7-bit parameter step.
 * Anchor the deadband at the last accepted sample so slow movement accumulates;
 * wheels retain the original three-count threshold. */
static inline int smk37_analog_moved(unsigned control, unsigned raw, unsigned previous)
{
    if(control<4 && raw!=previous && (raw==0 || raw>=1023))return 1;
    int delta=(int)raw-(int)previous;
    int threshold=control<4 ? 8 : 3;
    /* One unsigned interval check includes both movement directions. */
    return (unsigned)(delta+threshold-1)>=(unsigned)(2*threshold-1);
}
/* Provisional bench calibration from THIS unit. Never written to factory trim. */
static inline int16_t smk37_pitch_value(unsigned raw)
{
    if (raw <= 404) return -8192;
    if (raw >= 639) return 8191;
    if (raw < 524) return (int16_t)(-((int32_t)(524-raw)*8192/120));
    if (raw > 532) return (int16_t)((raw-532)*8191/107);
    return 0; /* 528 center, +/-4-count provisional deadband */
}
static inline uint8_t smk37_mod_value(unsigned raw)
{
    if (raw >= 630) return 0;
    if (raw <= 411) return 127;
    return (uint8_t)((630-raw)*127/219);
}
static inline uint8_t smk37_fader_value(unsigned raw)
{
    return raw >= 1023 ? 127 : (uint8_t)((raw*127+511)/1023);
}
typedef struct { uint16_t peak; uint8_t collecting, held; } smk37_pad_state_t;
typedef struct { uint8_t velocity, release; } smk37_pad_change_t;
/* Collect up to four conversions to retain the attack peak. The physical scan
 * cadence and all-pad sensitivity are still uncalibrated. A short tap emits
 * both attack and release; the consumer must preserve that ordering. */
static inline smk37_pad_change_t smk37_pad_sample(smk37_pad_state_t *s, unsigned raw)
{
    smk37_pad_change_t result = {0,0};
    if (raw > 1023) raw = 1023;
    if (!s->held && !s->collecting) {
        if (raw < 64) return result;
        s->collecting = 1;
        s->peak = (uint16_t)raw;
        return result;
    }
    if (s->collecting) {
        if (raw > s->peak) s->peak = (uint16_t)raw;
        if (++s->collecting >= 4 || raw <= 32) {
            unsigned v = s->peak >= 960 ? 127 : s->peak*127/960;
            result.velocity = (uint8_t)(v ? v : 1);
            s->collecting = 0;
            s->held = 1;
        }
    }
    if (s->held && raw <= 32) {
        s->held = 0;
        result.release = 1;
    }
    return result;
}
