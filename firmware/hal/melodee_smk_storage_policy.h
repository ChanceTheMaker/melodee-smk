/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
/* Shared by the RAM driver and host boundary tests. End addresses exclusive. */
#define MS_RANGE(a,n) ((uint32_t)(n)>0u && (((uint32_t)(a)>=0x9d000u&&(uint32_t)(a)<0xc0000u&&(uint32_t)(n)<=0xc0000u-(uint32_t)(a)) || ((uint32_t)(a)>=0xc2000u&&(uint32_t)(a)<0xff000u&&(uint32_t)(n)<=0xff000u-(uint32_t)(a))))
#define FL_RANGE_OK(a,n) (ms_device_ok && (ms_mounted||ms_formatting) && MS_RANGE(a,n))
