# Melodee SMK b1 device test

Public beta: 1.1.0-b1, update identity SMK-37_941.
Baseline: Melodee 01a78bd; measured SMK adapters from sloop-smk 090588d.

## Release state

The user reported that the earlier dev1 build boots and plays on their SMK. The new b1 storage, TRS and USB audio integration has not yet passed physical testing. Do not infer those results from the browser simulator or host tests. The owner requested publication of b1 as an experimental public beta. Hardware-test status remains false. The approved replacement build gate checks RAM placement and leaf control flow, and executes the production write-policy, power-loss and backup-gate tests before packaging. The public and local installers offer the exact reviewed b1 image. Publication is separate from physical validation; no device was flashed as part of this release task.

Candidate SHA-256: b03c3dfee7966e43e5f542a6c17c7f79e34df14283d8db83229fe3cc4abce40a. Application size: 581,252 bytes (312 bytes below the retained limit). Build audit: build/melodee-smk-1.1.0-b1/storage-audit.json.

## Storage ownership and preparation

Installing the application does not initialize its music storage. The fixed storage map is:

| Region | Purpose |
| --- | --- |
| 0x00000–0x9cfff | Boot/application: excluded from storage writes |
| 0x9d000–0xbffff | 35 data sectors |
| 0xc0000–0xc1fff | Factory calibration: excluded |
| 0xc2000–0xfdfff | 60 data sectors |
| 0xfe000–0xfefff | Storage ownership/layout marker |
| 0xff000–0xfffff | Excluded |

Each data sector leaves its final 256 bytes erased for the vendor boot scanner. Objects use CRC-checked linked sectors with the root committed last. The current valid generation stays allocated until a replacement commits. The bank payload format remains Melodee's native format; physical placement is SMK-specific.

The local storage.html tool downloads two byte-for-byte matching reads of the complete 393,216-byte region before enabling initialization. Every reply includes its offset and CRC. Initialization additionally checks the unit's calibration fingerprint, complete backup CRC, unchanged current contents, stopped playback, no held controls and no active voices. A final interrupt-protected quiet check locks synthesis until restart.

Initialization replaces existing music/sample data within those regions, including data belonging to stock or sibling firmware. The downloaded file is a raw storage backup, not a complete device recovery image. Retain native exports from the prior firmware too. There is no automatic format at boot or after an ordinary firmware update.

## Device checks, in order

1. Install the reviewed local candidate through normal USB, then confirm the Melodee SMK splash and b1 version. Retain prior exports and the known working installer.
2. Check all keys, pads, both knob/fader banks, wheels, sustain, soft buttons, octave changes and pad lights. Exercise each brightness level and screen sleep/wake.
3. With playback stopped and sound decayed, use storage.html to back up and explicitly initialize. Restart when instructed.
4. Save distinct music to A, B, C and D; restart; load each and check both sequence and sound. Repeat for a general preset, native FM6/CZ/Prophet preset, favorite and a changed setting. Check TRS enable survives restart. Never overwrite valuable slots for this test.
5. Connect TRS OUT to a receiver. Test single notes, chords, pedal, pitch and mod separately. Test internal clock and Start/Stop with a sequence. Document disconnect-while-held behavior. Continue and external-clock forwarding are not currently implemented by this integration.
6. Enable native USB audio in settings and reconnect if prompted. Verify stereo playback to the SMK and four independent mono track captures at 44.1 kHz, in both 16-bit and 24-bit modes. Record a repeating sequence for at least five minutes and inspect gaps, clipping and device health. This build does not claim 48 kHz support.
7. Verify Studio connects, edits sound, reads projects and restores a native backup; verify the USB firmware updater remains reachable.

## Automated evidence

- Storage host fixture: all native objects at maximum payload size, interrupted writes, CRC fallback, ownership checks and protected regions.
- Setup fixture: backup and checksum requirements, quiet-audio gate, bounds, explicit token, locked-until-restart behavior.
- Browser runtime: five engines, full keyboard/pad input, sustain and complete session restoration with corruption rejection.
- Native USB audio: endpoint lifecycle, descriptors, isolated real-mixer stems, PCM16/PCM24, packet bounds and simulated drift.

Record hardware results here before describing these integrations as device-tested or promoting the beta as a validated release. Bluetooth and user sample upload are outside this beta's completed feature set.

## b1 release verification — 2026-10-11

The exact published image passed Pro and Elite official-container round trips, protected-region comparisons and corrupt-image rejection. Storage policy/setup/torn-write fixtures pass. Native USB host checks pass PCM16/24 routing, 120-second simulated clock-drift cases, endpoint lifecycle, four isolated mixer stems and descriptors. These are automated host tests, not recordings or measurements from the physical SMK. Browser and installer regression suites also pass.
