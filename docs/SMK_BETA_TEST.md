# Melodee SMK dual-core beta

Public beta: **1.1.0-b1-dc7**, update identity `SMK-37_941`.
Baseline: Melodee 01a78bd; measured SMK adapters from sloop-smk 090588d.

## Release state

This is the exact application installed and tested on the owner's SMK-37 Pro.
FM6/Prophet manual playing and controlled capture exercised CPU1 without faults,
timeouts or rejected jobs. Projects A-D and general preset U1 survived restart.
44.1 kHz PCM16 USB capture worked for a short analog probe and one minute each
of FM6 and Prophet. Full evidence is in [SMK_DUAL_CORE.md](SMK_DUAL_CORE.md).

Known beta limitation: four-note FM6/Prophet tests each recorded 21 missed render
deadlines; the overload guard shed 77 and 40 voices respectively. Capture itself
reported no host errors, overruns, underruns or bad packets. This is not a claim
of glitch-free audio or twice the performance. Elite physical checks, longer
capture, USB playback/PCM24, physical MIDI expression/clock, native preset and
favorites persistence remain follow-up work. Bluetooth and sample uploads are
not available. Windows may retain the old Synthesizer (Melodee In) capture
name; new descriptors identify it as Line. If hidden, enable the Melodee recording
endpoint in Windows Sound settings; do not select an unrelated microphone.

The owner approved publishing this beta with these limits. `hardware_tested`
remains false to avoid implying complete model/feature coverage; the manifest
also records the specific partial Pro validation. Install protection retains
model/layout checks, calibration preservation and application-only write limits.
An interrupted installation can still require hardware recovery.

Application: 581,532 bytes (32 bytes below the retained limit).
SHA-256: `4426a29ad306a672464d778e44cb3a1238deae87d7b852dc86469f220987522c`.
Storage audit: `build/melodee-smk-1.1.0-b1-dc7/storage-audit.json`.

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

## Follow-up device checks

1. Install the reviewed local candidate through normal USB, then confirm the Melodee SMK splash and dc7 version. Retain prior exports and the known working installer.
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

Record new hardware results here; the checks above are not all completed. Bluetooth and user sample upload are outside this beta's completed feature set.

## b1 release verification — 2026-10-11

The exact published image passed Pro and Elite official-container round trips, protected-region comparisons and corrupt-image rejection. Storage policy/setup/torn-write fixtures pass. Native USB host checks pass PCM16/24 routing, 120-second simulated clock-drift cases, endpoint lifecycle, four isolated mixer stems and descriptors. These are automated host tests, not recordings or measurements from the physical SMK. Browser and installer regression suites also pass.
