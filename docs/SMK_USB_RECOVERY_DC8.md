# Melodee SMK 1.1.0-b1-dc8 USB updater repair

Selected public beta after owner-authorized Pro hardware validation on
October 12, 2026 (October 13 UTC). Runtime: `SMK-37_942`.

The public loader now accepts valid calibration or a wholly erased 4 KiB
USRTRIM sector, rejects partial corruption, and checks the entire sector after
updating. Application write bounds remain `0x4000..0x9d000`; no calibration is
created or written. This focused port comes from Doom `da621ca` via Sloop b9.
Existing synthesis, controls, storage layout, author credits and licenses remain.
No upstream contribution was submitted. Private backups and generated recovery
images remain under ignored build directories and are excluded from publication.

An instrument still running an older loader that rejects blank calibration can
require a one-time recovery-adapter repair. Publishing these builds does not
change an already-installed old loader. Old release binaries remain immutable.
Elite hardware validation remains pending.

Source baseline: dc7 `02671a8`, retaining public website work through `309a027`.
Only the SRAM updater uses `-Oz` to fit the existing application budget;
application/audio optimizations and the 8 KiB CPU1 stack are unchanged.

Application: 581,180 bytes (384 below the existing conservative limit), SHA-256
`5ff8fe4936b8a08175cd80bd6930efbb039f8efae38191034bba11ddf41531ff`.
RAM loader: 7,816 bytes. Linked SRAM, storage and dual-core audits passed.

Felucca 0.9.24 installed dc8 over normal USB with 2,450 requests, application
readback and exact 942 restart. A second, same-version normal-USB reinstall
also passed. The owner confirmed the screen, keys, knobs and sound afterward.
The first transition exercised changed application sectors; the second skips
unchanged sectors. Host simulation of the first transition changed 142 sectors
and preserved all protected bytes.

Public/private sanitizer tests and the complete multicore suite passed,
including 1,536 FM6 and 348 Prophet serial/concurrent comparisons, lifecycle
and fallback, USB services, audio, protected storage and initialization.
Prior dc7 workload evidence remains attributed to dc7. Long-duration dc8
USB-audio/storage hardware checks remain pending. The manifest's legacy
`hardware_tested: false` retains partial-beta compatibility; its detailed
hardware_validation record describes the completed Pro checks.
