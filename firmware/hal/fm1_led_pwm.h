/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* TIMER2 ends dim LED pulses inside each column's slot. It outranks TIMER5,
 * so USB service cannot stretch a dim pulse. Stop/clear it before changing
 * columns: its PORTA RMW must never interrupt the shift-register RMW. */
#pragma once
#include "fm1_time.h"
#include "fm1_gpio.h"

#ifndef FM1_LED_T2_CON
#define FM1_LED_T2_CON (*(volatile uint32_t *)0x10600u)
#define FM1_LED_T2_CNT (*(volatile uint32_t *)0x10604u)
#define FM1_LED_T2_PRD (*(volatile uint32_t *)0x10608u)
#endif

static void fm1__led_lines(uint32_t rowmask)
{
    uint32_t a = FM1_PR(FM1_PA, FM1_OUT) & ~((1u << 9) | (1u << 10));
    uint32_t h = FM1_PR(FM1_PH, FM1_OUT) & ~((1u << 6) | (1u << 9));
    FM1_PR(FM1_PA, FM1_OUT) = a | (rowmask & 2u) << 8 | (rowmask & 4u) << 8;
    FM1_PR(FM1_PH, FM1_OUT) = h | (rowmask & 8u) << 3 | (rowmask & 16u) << 5;
}

/* Rows remaining at the start, 1/8, 1/4 and 1/2 of the slot. OR preserves
 * the brightest requested level when planes overlap; 0xFF means dark. */
static void fm1_led_pwm_plan(uint8_t rows[4], uint8_t bright,
                             const uint8_t dim[3], const uint8_t masks[3])
{
    uint32_t i, j;
    for (j = 0; j < 4u; j++)
        rows[j] = bright;
    for (i = 0; i < 3u; i++) {
        uint32_t count = masks[i] == 0u ? 4u : masks[i] == 1u ? 3u :
                         masks[i] == 3u ? 2u : masks[i] == 7u ? 1u : 0u;
        for (j = 0; j < count; j++)
            rows[j] |= dim[i];
    }
}

static volatile struct {
    uint32_t start, span, phase;
    uint8_t rows[4];
} fm1_led_pwm;

static void fm1_led_pwm_cancel(void)
{
    FM1_LED_T2_CON = 0x4000u;                 /* stopped, pending cleared */
    fm1_led_pwm.phase = 4u;                  /* stale IRQ cannot use the old column */
}

static void fm1_led_pwm_advance(void)
{
    uint32_t p = fm1_led_pwm.phase;
    fm1_led_pwm_cancel();
    if (!p)
        return;                             /* harmless IRQ before the first pulse */
    while (p < 4u) {
        uint32_t due = fm1_led_pwm.span >> (4u - p);
        uint32_t elapsed = fm1_ticks() - fm1_led_pwm.start;
        if (fm1_led_pwm.rows[p] == fm1_led_pwm.rows[p - 1u]) {
            p++;
            continue;                        /* no IRQ for an unchanged level */
        }
        if (elapsed >= due) {
            fm1__led_lines(fm1_led_pwm.rows[p]);
            p++;
            continue;                        /* late IRQ: consume expired phases */
        }
        fm1_led_pwm.phase = p;
        FM1_LED_T2_CNT = 0;
        FM1_LED_T2_PRD = due - elapsed;
        FM1_LED_T2_CON = 0x4009u;             /* OSC /1: same 24 MHz as TIMER4 */
        return;
    }
    fm1_led_pwm.phase = 4u;
}

static void fm1_led_pwm_begin(const uint8_t rows[4], uint32_t span)
{
    uint32_t i;
    for (i = 0; i < 4u; i++)
        fm1_led_pwm.rows[i] = rows[i];
    fm1_led_pwm.span = span;
    fm1_led_pwm.phase = 1u;
    fm1_led_pwm.start = fm1_ticks();
    fm1__led_lines(rows[0]);
    fm1_led_pwm_advance();
}
