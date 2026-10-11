/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* The actual LED planner, pulse ISR and input tick against register stubs.
 * Exercise every column/frame, overlapping brightness requests, late IRQs,
 * timer wrap, cancellation and GPIO preservation. No physical timing claim. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t now, ports[8][16], t2_con, t2_cnt, t2_prd;
#define FM1_T4_CNT now
#define FM1_PR(p, r) ports[p][(r) / 4u]
#define FM1_LED_T2_CON t2_con
#define FM1_LED_T2_CNT t2_cnt
#define FM1_LED_T2_PRD t2_prd
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#include "../firmware/hal/fm1_input.h"

static int fails;
static void check(const char *name, int ok)
{
    printf("%-70s %s\n", name, ok ? "ok" : "FAIL");
    fails += !ok;
}

static uint32_t rows_on(void)
{
    return ((ports[FM1_PA][FM1_OUT / 4u] >> 8) & 6u) |
           ((ports[FM1_PH][FM1_OUT / 4u] >> 3) & 8u) |
           ((ports[FM1_PH][FM1_OUT / 4u] >> 5) & 16u);
}

static void clear(void)
{
    uint32_t i;
    now = 0;
    memset(ports, 0, sizeof ports);
    memset((void *)&fm1_in, 0, sizeof fm1_in);
    memset((void *)&fm1_in_stat, 0, sizeof fm1_in_stat);
    memset((void *)fm1_led, 0, sizeof fm1_led);
    memset((void *)fm1_led_dim, 0, sizeof fm1_led_dim);
    memset((void *)fm1_led_dim_mask, 0, sizeof fm1_led_dim_mask);
    memset((void *)&fm1_led_pwm, 0, sizeof fm1_led_pwm);
    fm1__tick_col = 0;
    for (i = 0; i < FM1_NENC; i++)
        fm1_in.enc_prev[i] = fm1_in.enc_last[i] = 0xFF;
    ports[FM1_PA][FM1_IN / 4u] = 0xFFFFFFFFu;
    ports[FM1_PB][FM1_IN / 4u] = 0xFFFFFFFFu;
    fm1_led_pwm_cancel();
}

/* Model independently: choose the largest duty requested for each row. */
static uint32_t duty(uint32_t mask)
{
    return mask == 0 ? 8u : mask == 1 ? 4u : mask == 3 ? 2u : mask == 7 ? 1u : 0u;
}

int main(void)
{
    static const uint8_t levels[] = {0, 1, 3, 7, 0xFF};
    uint32_t a, b, c, row, stage, ok = 1;
    uint8_t plan[4], dim[3], masks[3];
    clear();
    fm1_led_pwm.phase = 0u;
    fm1_led_pwm_advance();
    check("pulse IRQ before initialization is harmless", !(t2_con & 1u) && rows_on() == 0u);
    /* Exhaust all level combinations and all combinations of overlapping
     * bright/dim planes on a row, against the independent maximum-duty model. */
    for (a = 0; a < 5u; a++)
        for (b = 0; b < 5u; b++)
            for (c = 0; c < 5u; c++) {
                masks[0] = levels[a]; masks[1] = levels[b]; masks[2] = levels[c];
                for (row = 0; row < 16u; row++) {
                    uint32_t want = row & 8u ? 8u : 0u, i;
                    for (i = 0; i < 3u; i++) {
                        uint32_t d = row & (1u << i) ? duty(masks[i]) : 0u;
                        dim[i] = row & (1u << i) ? 2u : 0u;
                        if (d > want) want = d;
                    }
                    fm1_led_pwm_plan(plan, row & 8u ? 2u : 0u, dim, masks);
                    for (stage = 0; stage < 4u; stage++) {
                        static const uint32_t units[] = {0, 1, 2, 4};
                        ok &= !!(plan[stage] & 2u) == (want > units[stage]);
                    }
                }
            }
    check("all brightness combinations preserve the brightest overlapping plane", ok);

    dim[0] = 2u; dim[1] = 4u; dim[2] = 8u;
    masks[0] = 7u; masks[1] = 3u; masks[2] = 1u;
    fm1_led_pwm_plan(plan, 16u, dim, masks);
    ports[FM1_PA][FM1_OUT / 4u] = 1u << 12;
    ports[FM1_PH][FM1_OUT / 4u] = 1u << 8;
    fm1_led_pwm_begin(plan, 2400u);
    check("every level starts lit, first pulse ends at 12.5 us", rows_on() == 30u && t2_prd == 300u && (t2_con & 1u));
    now = 300u; fm1_led_pwm_advance();
    check("eighth pulse ends; quarter, half and bright continue", rows_on() == 28u && t2_prd == 300u);
    now = 600u; fm1_led_pwm_advance();
    check("quarter pulse ends at 25 us; half and bright continue", rows_on() == 24u && t2_prd == 600u);
    now = 1200u; fm1_led_pwm_advance();
    check("half pulse ends at 50 us; bright stays on without another IRQ", rows_on() == 16u && !(t2_con & 1u));
    check("pulse ISR preserves unrelated PORTA and PORTH output bits", (ports[FM1_PA][FM1_OUT / 4u] & (1u << 12)) &&
          (ports[FM1_PH][FM1_OUT / 4u] & (1u << 8)));

    now = 0; fm1_led_pwm_begin(plan, 2400u);
    now = 1000u; fm1_led_pwm_advance();
    check("late IRQ consumes expired phases without stretching the next deadline", rows_on() == 24u && t2_prd == 200u);
    now = 5000u; fm1_led_pwm_advance();
    check("IRQ after the slot expires leaves only bright rows and stops", rows_on() == 16u && !(t2_con & 1u));
    now = 0; fm1_led_pwm_begin(plan, 2400u);
    fm1_led_pwm_cancel();
    now = 1200u; fm1_led_pwm_advance();
    check("cancellation invalidates all old-column pulse deadlines", rows_on() == 30u && !(t2_con & 1u));
    now = UINT32_MAX - 100u; fm1_led_pwm_begin(plan, 2400u);
    now += 600u; fm1_led_pwm_advance();
    check("pulse deadlines remain correct across TIMER4 wrap", rows_on() == 24u && t2_prd == 600u);

    clear();
    fm1_led_dim_mask[1] = 3u;
    for (a = 0; a < FM1_NCOL; a++) fm1_led_dim[1][a] = 2u;
    ok = 1;
    for (a = 0; a < 88u; a++) {
        uint32_t start = now;
        fm1_input_tick();
        ok &= rows_on() == 2u && t2_prd == 600u && (t2_con & 1u);
        now = start + 600u; fm1_led_pwm_advance();
        ok &= rows_on() == 0u && !(t2_con & 1u);
        now = start + 2400u;
    }
    check("idle button quarter pulses repeat in every column of every frame", ok && fm1_in.frames == 8u);

    clear();
    fm1_led_dim_mask[1] = 3u;
    fm1_led_dim[1][1] = 2u;
    fm1_led[2] = 4u;
    fm1_input_tick();                         /* column 1 dim, pulse armed */
    now = 2400u;
    fm1_input_tick();                         /* cancel stale pulse, column 2 bright */
    check("column change cancels the old pulse before lighting the next column", rows_on() == 4u && !(t2_con & 1u));
    fm1_led_pwm_advance();                    /* harmless duplicate IRQ */
    check("a cleared/duplicate pulse IRQ cannot blank the next bright column", rows_on() == 4u && !(t2_con & 1u));
    dim[0] = dim[1] = dim[2] = 0;
    fm1_led_pwm_plan(plan, 0u, dim, masks);
    fm1_led_pwm_begin(plan, 2400u);
    check("dark columns schedule no pulse interrupts", rows_on() == 0u && !(t2_con & 1u));
    printf("LED PWM: %d failure(s)\n", fails);
    return fails != 0;
}
