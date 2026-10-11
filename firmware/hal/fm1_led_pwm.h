/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* WL82 MCPWM1/3, independent H/L outputs, for the four matrix LED rows.
 * GPIO routing and PWM setup follow AC79 SDK cpu.a gpio.c / pwm.c and
 * asm/gpio.h / WL82.h. Do not use MCPWM0/2: their fixed output pins are
 * the shift register and LCD. MCPWM1's fixed PA7/8 remain matrix INPUTS;
 * MCPWM3's unused fixed PH0/1 also remain inputs. Only the routed LED pins
 * are outputs. No PWM interrupt or software pulse-end deadline is used.
 *
 * 64 LSB clocks per carrier (~1 us with the board's 60 MHz LSB clock).
 * Zero compares blank the rows; allow 4 us for the next carrier boundary
 * before changing columns, including hardware with a slower LSB clock.
 * Pin routes are installed once, with all columns deselected. Runtime
 * updates touch compares only: no GPIO mux transitions or startup pulses.
 */
#pragma once
#include <stdint.h>
#include "fm1_gpio.h"
#include "fm1_time.h"

#ifndef FM1_LED_PWM_REG
#define FM1_LED_PWM_REG(a) (*(volatile uint32_t *)(uintptr_t)(a))
#endif
#ifndef FM1_LED_PWM_NOW
#define FM1_LED_PWM_NOW() fm1_ticks()
#endif
#define FM1_LED_PWM_PERIOD 64u
#define FM1_LED_PWM_BLANK_TICKS (4u * FM1_TICKS_PER_US)
#define FM1_LED_PWM_CH1 0x15074u
#define FM1_LED_PWM_CH3 0x15094u

static void fm1_led_pwm_write(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    FM1_LED_PWM_REG(FM1_LED_PWM_CH3 + 8u) = a;  /* PA9: output ch0, MCPWM3 H */
    FM1_LED_PWM_REG(FM1_LED_PWM_CH3 + 12u) = b; /* PA10: output ch1, MCPWM3 L */
    FM1_LED_PWM_REG(FM1_LED_PWM_CH1 + 8u) = c;  /* PH6: output ch2, MCPWM1 H */
    FM1_LED_PWM_REG(FM1_LED_PWM_CH1 + 12u) = d; /* PH9: output ch3, MCPWM1 L */
}

static void fm1_led_pwm_blank(void)
{
    uint32_t t0;
    fm1_led_pwm_write(0, 0, 0, 0);
    t0 = FM1_LED_PWM_NOW();
    while ((uint32_t)(FM1_LED_PWM_NOW() - t0) < FM1_LED_PWM_BLANK_TICKS)
        ;
}

/* Masks retain the UI's 0/1/3/7 levels; 0xff means dark, including BREATH. */
static uint32_t fm1_led_pwm_level(uint32_t row, uint32_t bright,
                                const uint8_t dim[3], const uint8_t mask[3])
{
    uint32_t i, level = 0;
    if (bright & row)
        return FM1_LED_PWM_PERIOD;
    for (i = 0; i < 3u; i++)
        if (dim[i] & row) {
            uint32_t duty = FM1_LED_PWM_PERIOD / ((uint32_t)mask[i] + 1u);
            if (duty > level)
                level = duty;
        }
    return level;
}

static void fm1_led_pwm_init(void)              /* IRQs off, all columns deselected */
{
    const uint32_t a = (1u << 9) | (1u << 10), h = (1u << 6) | (1u << 9);
    uint32_t i;
    /* Preserve other MCPWM channels and all unrelated GPIO / IOMAP bits. */
    FM1_LED_PWM_REG(0x150E4u) &= ~0xA0Au;
    FM1_PR(FM1_PA, FM1_DIR) |= (1u << 7) | (1u << 8);
    FM1_PR(FM1_PH, FM1_DIR) |= 3u;
    for (i = 1u; i <= 3u; i += 2u) {
        uint32_t timer = 0x15000u + 12u * i, ch = 0x15064u + 16u * i;
        FM1_LED_PWM_REG(timer) = 1u;            /* edge aligned, LSB /1, no IRQs */
        FM1_LED_PWM_REG(timer + 8u) = FM1_LED_PWM_PERIOD;
        FM1_LED_PWM_REG(timer + 4u) = FM1_LED_PWM_PERIOD;
        FM1_LED_PWM_REG(ch) = 0xEu;            /* independent H/L, both enabled */
        FM1_LED_PWM_REG(ch + 4u) = 0x4000u | (i << 8); /* select its timer, clear pending */
    }
    fm1_led_pwm_write(0, 0, 0, 0);
    FM1_LED_PWM_REG(0x150E4u) |= 0xA0Au;       /* channels 1/3 and timers 1/3 */
    fm1_led_pwm_blank();

    FM1_LED_PWM_REG(0x51020u) = (FM1_LED_PWM_REG(0x51020u) & ~(15u << 8)) | (12u << 8);
    FM1_LED_PWM_REG(0x51028u) = (FM1_LED_PWM_REG(0x51028u) & ~0xFFF00000u) | 0xBAD00000u;
    /* OUT and DIE encode the output-channel number when PU and PD are both 1:
     * ch0 = 0/0, ch1 = 0/1, ch2 = 1/0, ch3 = 1/1. */
    FM1_PR(FM1_PA, FM1_OUT) &= ~a;
    FM1_PR(FM1_PA, FM1_DIE) = (FM1_PR(FM1_PA, FM1_DIE) & ~a) | (1u << 10);
    FM1_PR(FM1_PH, FM1_OUT) |= h;
    FM1_PR(FM1_PH, FM1_DIE) = (FM1_PR(FM1_PH, FM1_DIE) & ~h) | (1u << 9);
    FM1_PR(FM1_PA, FM1_PD) |= a;
    FM1_PR(FM1_PH, FM1_PD) |= h;
    FM1_PR(FM1_PA, FM1_PU) |= a;
    FM1_PR(FM1_PH, FM1_PU) |= h;
}
