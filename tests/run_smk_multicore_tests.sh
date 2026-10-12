#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Local host checks; generate headers with build_melodee_smk.py --generate first.
set -eu
cd "$(dirname "$0")/.."
OUT=build/smk-multicore-tests
mkdir -p "$OUT"
CC=${HOST_CC:-cc}
for mode in 0 1; do
 "$CC" -std=c11 -Wall -Wextra -Werror -DMELODEE_DUAL_CORE=$mode tests/melodee_smk_multicore_test.c -o "$OUT/lifecycle-$mode"
 "$OUT/lifecycle-$mode"
done
"$CC" -O2 -Wall -Wno-unused-function tests/audio_worker_test.c -o "$OUT/worker"
"$OUT/worker"
for test in dual_core_test melodee_smk_render_worker_test; do
 "$CC" -O2 -ffp-contract=off -fno-builtin -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -pthread "tests/$test.c" -lm -o "$OUT/$test"
done
"$OUT/dual_core_test"
"$OUT/melodee_smk_render_worker_test" 0 "$OUT/serial.pcm"
"$OUT/melodee_smk_render_worker_test" 1 "$OUT/concurrent.pcm"
cmp "$OUT/serial.pcm" "$OUT/concurrent.pcm"
printf 'SMK serial/concurrent PCM: byte-identical\n'
for test in smk_usb_timer smk_usb_descriptor smk_fader_deadband smk_usb_service smk_audio_budget audio melodee_smk_storage_policy melodee_smk_storage melodee_smk_setup usb_audio usb_audio_driver usb_audio_tracks; do
 "$CC" -O2 -std=gnu11 -w -Ibuild/gen -Ifirmware/src "tests/${test}_test.c" -lm -o "$OUT/$test"
 "$OUT/$test"
done
python3 tests/smk_storage_audit_test.py
python3 tests/smk_multicore_audit_test.py
