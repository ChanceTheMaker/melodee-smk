/* SPDX-License-Identifier: GPL-3.0-only */
/* The hardware trial is opt-in. Browser and baseline builds remain serial.
 * Shared with startup assembly so CPU1's entry and the C runtime agree. */
#pragma once
#ifndef MELODEE_DUAL_CORE
#define MELODEE_DUAL_CORE 0
#endif
#ifndef MELODEE_CACHE_RAM
#define MELODEE_CACHE_RAM 0
#endif
