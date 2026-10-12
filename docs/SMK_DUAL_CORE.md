# Melodee SMK dual-core trial

Local development branch: `feature/smk-dual-core`, based on `7e3c002`.
The approved public beta is **1.1.0-b1-dc7**. The exact tested application is pinned by hash for publication.
The preceding dc1 build ran its worker successfully on the owner's Pro, with
clean manual FM6 and Prophet tests and zero worker faults/timeouts. The dc2
build adds the upstream audio overload guard. The Pro completed its first
controlled dc2 run; occasional missed audio deadlines remain under review. The owner approved publishing dc7 with the remaining limitations documented.
The normal build and browser runtime still default to one core.

## What the second core does

This reuses upstream Melodee's AC79 worker, mailbox validation and paired FM6 /
Prophet kernels. CPU0 owns keys, pads, controllers, sequencing, effects, USB,
display and flash. CPU1 renders eligible paired voices; each job is joined
before the next control block returns. Other engines stay on CPU0. This is
not a promise of twice the overall performance or of increased RAM.

No PLL, voltage, cache-as-RAM, flash layout, storage ownership or calibration
policy changes are made. CPU1 gets separate 4 KiB user and supervisor stacks.
It is held before startup touches application RAM, and before the updater
copies its RAM loader. A startup handshake has a 10 ms bound. Stalls, rejected
mailbox requests and CPU1 faults retire the worker; the serial renderer takes
over. Failed startup retries remain serial. Retired workers stay barred across
warm resets, with a new attempt allowed after a detected cold power-on.

The idle worker resides entirely in copied SRAM, with only its job callback
allowed as an indirect call. At a completed render boundary it cannot fetch
flash while the main thread performs NOR operations. The linked-image audit
rejects misplaced polling code, direct helper calls, escaping branches,
stack overlap and insufficient working RAM. The existing SMK storage audit
still checks leaf flash routines and protected write ranges.

## Reproduce locally

Use the existing JieLi Linux toolchain and the font-generation dependencies
from BUILDING.md. From this checkout, with JIELI_TOOLCHAIN set:

~~~sh
JIELI_DOCKER=0 python3 tools/build_melodee_smk.py --dual-core --generate
sh tests/run_smk_multicore_tests.sh
JIELI_DOCKER=0 python3 tools/build_melodee_smk.py
~~~

The final command builds a serial comparison image from these sources; it is
not a byte-identical copy of the published b1 image. Variant-specific generated
unit/loader headers live inside each output directory. Neither command stages
web assets. Application images are raw candidate binaries for the guarded SMK
packaging/update flow, not stock .fwsc files or full-flash backups.

| Original dc1 artifacts | Dual-core trial | Serial comparison |
| --- | --- | --- |
| Directory under build/ | melodee-smk-1.1.0-b1-dc1 | melodee-smk-1.1.0-b1 |
| Candidate bytes | 580,860 | 579,076 |
| Fixed application limit | 581,564 | 581,564 |
| CPU1 stack bytes | 8,192 | 0 |
| Main audio working arena | 157,824 | 166,016 |
| Physical status | Manual FM6/Prophet passed; see comparison below | Controlled comparison completed |

The original dc1 trial had only 704 bytes of application headroom and 724 bytes remaining
in the main data/BSS bank. It must keep passing the linker and resource audits
as code evolves. To fit, the SMK build shares UI/formatting/LCD helper routines
instead of duplicating them inline, and size-optimizes additional startup and
store wrapper functions. Render kernels, audio/USB/scanner IRQ entry-point
optimization and RAM flash leaf functions retain their existing treatment.
The upstream FM-1 build does not receive the SMK-specific size options.

## Checks completed on 2026-10-12

- Both target variants compile/link within the existing flash and SRAM limits.
- The target audit finds a 42-instruction RAM polling body, one register-held
  job callback, private aligned CPU1 stacks and enough audio working arena.
- 1,536 FM6 and 348 Prophet upstream serial/concurrent render cases match,
  including busy/offline behavior, sequence wrap and retirement mid-song.
- The real SMK runtime produces byte-identical PCM with dispatch disabled and
  with a concurrent worker: 3,160 completed jobs, including physical key/pad,
  sustain, pitch/mod and fader events. No worker timeout or rejected request.
- Worker tests cover corrupt requests, timeout wrap, stalls and fault retirement.
- SMK lifecycle tests cover serial builds, handshake failure, warm/cold restart,
  failed-boot suppression, worker retirement and intentional updater shutdown.
- SMK storage policy, torn-write/CRC fallback and setup/backup guards pass.
- USB PCM16/24, simulated clock drift, endpoint recovery and track isolation pass.
- All upstream UI behavior tests pass.
- Seven flash-audit and eleven worker-audit tests pass, including rejection of
  unsafe helper calls, worker placement and stack overlap.

Host timing is not an SMK speed measurement. No USB device was opened or
modified for these tests.

## Device validation still required

### Installation evidence, 2026-10-12

The private `build/adapter-dc1-20261012T160152228Z/` record contains two matching
pre-install backups, a protected-range comparison, 142 verified application
sector writes, and the complete verified final image. Candidate SHA-256:
`a250c9b517beb1824d3b3cd100037d33e7079924e839f54ecfd57ba7c087f7c6`.
The final device-image SHA-256 was
`95147546528d2c3803700219d9955e66160c2b4de71e49bb848ccf0f9ae71f63`.
This establishes successful programming, not a runtime or audio performance pass.
The SMK subsequently received Doom Test07, then successfully installed dc1
through normal USB using this installer. It also installed the single-core
comparison through normal USB on 2026-10-12. The local installer now accepts the model-specific Doom SMK
Test07+ loader, retaining exact loader handoff and returned Melodee identity
checks. It rejects Doom FM-1 and older Doom loaders before programming.

Local trial page: `http://localhost:8792/installer.html`. Its dual-core candidate
is restricted to loopback hosts and the pinned application hash. Public release
files and device music storage are not changed by preparing this page.

Use a backed-up development SMK with a known recovery path for the first trial.
Do not release the candidate based only on host results.

1. Verify normal startup, controls, audio and USB enumeration. Confirm the trial
   version and query the existing read-only audio/worker statistics:

   ~~~sh
   python tools/usb_audio_stats.py --port Melodee --cores --voices --memory --seconds 120
   ~~~

2. Play sustained FM6 and Prophet chords. Expect core1_online=1 and core1_jobs
   increasing when voices can be paired. An idle instrument or another engine
   is not a valid speed test. Rejected requests, faults and timeouts should stay
   at zero; monitor render time, late audio and USB counters as well.
3. Compare the same patches, voice count, effects and USB capture session with
   the serial comparison build. Check dropouts, pitch/mod, sustain, display
   responsiveness and long playback, not only the CPU percentage.
4. Save and reload a disposable project/preset, then power-cycle and verify it.
   Keep the usual explicit backup gate for storage initialization.
5. Validate a normal direct-USB update back to the known-good release before
   considering release. Do not inject CPU faults on a live user's instrument.

Felucca and Sloop have not been changed by this branch. Porting the worker to
those engines is separate work; their audio state and render boundaries need
their own concurrency design and tests.

## Physical comparison, 2026-10-12

Private evidence is in ignored `build/` files; no device backups are published.

- `dc1-fm6-live-20261012.jsonl`: 90 seconds, 51,518 worker jobs, up to eight
  active voices, no additional late audio blocks, faults, rejects or timeouts.
  Owner reported clean sound and responsive controls.
- `dc1-prophet-live-20261012.jsonl`: 90 seconds, 59,924 worker jobs, up to three
  active voices, no additional late blocks or worker errors. Owner reported
  clean sound and responsive controls.
- `dc1-controlled-20261012/` and `serial-controlled-20261012/`: 20 repetitions
  per engine of notes 60/64/67/71, velocity 90, two seconds held/one released,
  engine INIT patches. CPU medians below include samples with exactly four
  held voices. USB audio capture was not active.

| Engine | Serial CPU median | dc1 CPU median | Serial late blocks | dc1 late blocks |
| --- | ---: | ---: | ---: | ---: |
| FM6 | 63.25% | 52.15% | 16 | 0 |
| Prophet | 101.15% | 73.4% | 7,855 | 101 |

These are CPU0 workload counter readings, not total two-core CPU usage or a
polyphony guarantee. The owner confirmed playing along during the dc1 run;
its Prophet run peaked at five held voices. This limits direct attribution of
its late-block total, though the four-held samples show substantially less
CPU0 work. Serial stayed at four held voices, with CPU1 offline throughout.
Both dc1 controlled runs had zero worker faults, rejected jobs or timeouts.

The attempted four-channel Windows USB capture failed to open with PortAudio
`Invalid device (-9996)`; no audio recording was obtained. The input appeared
only under WDM-KS. USB capture quality and sustained streaming remain pending.

Read-only storage status showed a valid device but an uninitialized Melodee
store. No storage initialization or music-data writes were performed. Obtain
two matching private backups and explicit approval before replacing existing
music/sample storage, then test saves across a power cycle.

## dc2 overload protection

SMK was missing the upstream IRQ's two-consecutive-block, 85% overload policy.
The SMK IRQ now uses that policy and the shared upstream voice selection:
fade a quiet released tail first, otherwise the oldest eligible held voice,
preserving the mono lead and poly bass. Nested interrupt time counts toward
the DMA deadline. This is overload protection, not increased guaranteed
polyphony or a promise of zero missed deadlines.

The shared implementation leaves the upstream FM-1 selection unchanged.
Host tests cover isolated spikes, sustained overload, tail selection and
protected voices; upstream audio, paired-kernel parity, runtime worker,
storage, USB and target safety audits pass.

The dc2 application is 581,532 bytes, leaving **32 bytes** of the fixed
581,564-byte app region. Main-loop editor reply helpers are shared rather
than inlined to fit; flash boundaries and storage layout remain unchanged.
SHA-256: `e63d40ba123691e0ee5ba5301808b3cdd2d5143221bcaef4c5c84a00bee3a935`.
Do not release without physical dc2 tests. This image has effectively no
flash headroom for further features without additional size work.

Local dual-core installer: `http://localhost:8792/installer.html`.
Original serial comparison: `http://localhost:8792/serial/installer.html`.
The serial route retains the audited original b1 image in memory, and is not
rebuilt or modified by staging dc2.

## dc2 first device run, 2026-10-12

Direct USB install succeeded from the serial comparison. INFO returned
`MELODEE SMK 1.1.0-b1-dc2`; CPU1 was online. The controlled four-note run
(`build/dc2-controlled-20261012/`) completed:

| Engine | Seconds | Worker jobs | Additional late blocks | Voices faded by overload guard |
| --- | ---: | ---: | ---: | ---: |
| FM6 | 63.578 | 152,713 | 1 | 0 |
| Prophet | 64.828 | 103,372 | 4 | 19 |

Both had zero worker rejects, faults, timeouts or resource allocation failures.
FM6 held four voices; Prophet sometimes retained three after shedding. Held
CPU medians were 53.9% and 72.1%, respectively, but Prophet is no longer an
identical sustained-polyphony comparison to dc1. The guard is physically
observed working. This is not a zero-dropout pass: occasional late blocks
remain, and USB audio streaming was inactive.

## Storage transfer race and dc3

A five-note dc2 exercise completed with the worker still online and no worker
faults, rejects or timeouts (`build/dc2-five-note-20261012/`). Overload shedding
activated; some late blocks remained. No zero-dropout claim is made.

A read-only raw-storage test timed out. The instrument still answered INFO
and audio diagnostics. Two complete reads then matched using bounded read
retries: 393,216 bytes each, SHA-256
`52d7b4355e759f91efad843d814d5181f4e64fc279ef4bb962dd7c1b42883ffe`.
The private files are `build/dc2-storage-retry-20261012/pass-1.bin` and
`pass-2.bin`; eight chunk retries were needed. No flash writes were sent.
Because a repeated chunk invalidates the firmware's sequential setup gate,
these backups do not authorize or unlock initialization. A fresh uninterrupted
read is required in addition to explicit owner approval.

Review found `melodee_usb_service` could clear a newly arrived request after
`ed_service` returned. The regression test reproduces this on the old code,
including when the queue was initially empty. The new dispatcher classifies
unknown packets before editor handling and never clears a later packet.
It preserves the storage-setup lock, identity reply and storage namespace.
The dispatcher is factored into a header so the exact production function is
exercised by `tests/smk_usb_service_test.c`. dc3 is a local follow-up to test
whether this fix eliminates the observed device transfer timeouts.

Final dc3 candidate: 581436 bytes, SHA-256 `d4d2d1253528fae54884b5d1ebff0c945ad242e6a353b713a295981cc7617670`.
Target storage/worker audits and the host suite passed. Device validation is pending.

## dc3 storage validation, 2026-10-12

The owner installed dc3 through normal USB. INFO returned the exact dc3
version, CPU1 was online, and idle worker fault/reject/timeout counters were
zero. Two full raw storage reads completed with **zero retries across 3,072
chunks**. Both copies match each other and the earlier dc2 backup byte for
byte. Evidence: private `build/dc3-storage-20261012/manifest.json` and two
393,216-byte files. No initialization or flash writes were sent.

This validates the transfer fix for this Pro and this run. The storage remains
uninitialized. The prepared initialization plan covers only the reviewed
music/sample area (0x9d000..0xbffff and 0xc2000..0xfefff), preserving application,
bootloader and factory calibration. Explicit owner approval is pending before
replacing that saved data with empty Melodee storage. A power-cycle and actual
save/reload tests must follow initialization.

`tools/smk_storage_initialize.py --backup <private-backup-folder>` is a dry-run
plan. Execution requires `--execute --journal <new-private-file>` after owner
approval; it validates both files, checks the device fingerprint and quiet
state, sends initialization once and never retries it automatically.

## Approved storage initialization, 2026-10-12

After the owner explicitly approved replacing the backed-up save area, the
initialization tool revalidated both local backups, the connected device's
fingerprint/layout, unmounted state and quiet audio. It sent exactly one
initialization request. The device returned code 0 (success) after about two
seconds. Journal: private `build/dc3-storage-initialize-20261012.jsonl`.

The device must now be power-cycled to mount the new storage. Mount status,
project/preset saves, reloads and persistence across a subsequent restart
remain to be tested. The pre-initialization backups remain available unchanged.

## First mounted save test, 2026-10-12

After the owner's restart, storage reported mounted. Projects A-D and user
preset U1 were empty. The scripted trial saved four distinct sounds (analog,
FM6, Prophet, analog) with distinct first sequencer notes, then loaded each
project and compared every standard sound parameter and the first step.
All four matched. U1 `DC3 SAVE A` saved, read back identically and loaded.
CPU1 remained online, with zero worker faults/rejects/timeouts. Evidence:
private `build/dc3-save-trial-20261012/expected.json` and
`immediate-result.json`. Full project/preset persistence awaits the next
owner-operated power cycle; deep native parameter coverage is not claimed.

## Restart investigation and test correction

On the next restart A-C initially matched, while D retained its step but had
U1's sound. U1 itself read back unchanged. The test had loaded U1 while D was
the current project; upstream Melodee automatically saves edited projects
after five idle seconds. This made the test change its own D fixture. The
test now reloads D immediately after checking U1 to leave the project intact.
The owner also reported a key had inadvertently been held during the check.

After restoring D's intended sound, repeated loads showed a separate +/-1
change in P_LEVEL (68/69). The firmware forwards analog changes of only three
ADC counts to the level faders. That is smaller than a 7-bit fader step and
can feed ADC noise into project autosave. dc4 raises only the four fader
thresholds to eight raw counts, anchored at the last accepted position so
slow movement accumulates, with explicit endpoint delivery. Pitch and mod
retain three-count sensitivity. Host tests cover stationary jitter, slow
movement, reverse movement, exact endpoints and unchanged wheel sensitivity.
Hardware stability and final restart verification remain pending.

No general persistence pass is claimed yet. The D fixture was intentionally
repaired after diagnosing autosave; prior saved test data and investigation
logs remain in the ignored private test directory.

Local dc4 candidate: 581500 bytes, SHA-256 `5eae1ca0260c224cc37b51ed346c678675035b2f4fe2ac01d93e4d48b15dc6ba`.
The existing flash layout and initialized music storage are unchanged by this application update.

## dc4 hardware result and dc5 fader smoothing

The owner installed dc4 successfully. All four saved project sounds/steps
and U1 were present; however ten repeated sound reads per project showed
P_LEVEL still varying, usually 68/69 and once 67. The eight-count deadband
alone did not deliver stable idle levels. Evidence: private
`build/dc4-save-check-20261012/initial.json`. Other standard sound fields
remained unchanged during those repeated reads.

dc5 adds a per-fader quarter-rate integer low-pass at the ADC producer, only
on fresh conversions. Startup initializes from the actual position rather
than zero. Fractional history is retained, and stable input converges to both
exact endpoints. The existing eight-count movement gate follows the filter;
pitch/mod and pads are not filtered by it. Tests cover a synthetic 23-count
stationary-noise span, slow movement and exact endpoint convergence. These
synthetic inputs are stress fixtures, not recorded raw ADC measurements.

Main-loop FM6 parameter formatting and page helpers use size optimization.
The fader filter uses one biased accumulator instead of a separate validity
flag; its movement gate is exhaustively checked over all 10-bit input pairs
and six control indices. No flash-layout expansion is used.
Hardware confirmation of the smoothing remains required.

Local dc5 candidate: 581564 bytes (the exact application limit),
SHA-256 `c053dbacfe1900e1be4b9ba049aef6bbbefdee33bb11119520701ac3eb3acb2a`. Flash and CPU1 audits pass.
The host suite passes; the final compact filter also passes its focused
regression and exhaustive movement-gate comparison. Physical idle stability
and corrected persistence verification remain pending.

## dc5 first physical check, 2026-10-12

The owner installed dc5. Identity matched, initialized storage was mounted,
and CPU1 remained online with no worker faults or timeouts. No held/active
voices were present. An 80-read initial idle capture recorded one P_LEVEL
change from 104 to 69 at sample 44 (about 11 seconds); its cause is not yet
established, so complete idle-noise suppression is not claimed.

After loading each of A-D, 24 repeated sound reads per project were stable,
with P_LEVEL 69. Every standard sound parameter and first sequencer step
matched the original saved fixture. U1 read back identically. The corrected
verification tool also loaded U1 and immediately restored D to avoid its
prior autosave interference. Evidence remains private under
`build/dc5-idle-20261012-105841.json`,
`build/dc5-project-check-20261012-105938.json`, and
`build/dc3-save-trial-20261012/restart-result.json` (dc5 diagnostics).
A subsequent owner power cycle and longer fader stability check remain open.

## dc5 power-cycle pass and Windows capture diagnosis

The owner power-cycled dc5. The corrected verification matched every standard
sound parameter and first step in projects A-D; U1 read back identically and
loaded. CPU1 was online. The following 160-read idle observation (about 40 s)
had zero changing sound bytes, a steady level of 69, zero worker faults and
zero timeouts. This passes the scoped Pro save/restart test, not full deep
engine serialization coverage or an indefinite ADC-noise guarantee. Evidence:
`build/dc5-post-restart-20261012-110107.json` and the save trial restart result.

Windows still exposes capture only through WDM-KS. The USB descriptor uses
terminal 0x0713 (synthesizer); Microsoft explicitly lists that category as
hidden/disabled by default. dc6 changes ONLY the SMK capture terminal to
0x0603 (line connector), retaining four non-spatial mono tracks, PCM16/24,
44.1 kHz, endpoint sizes, feedback, and the original FM-1 descriptors.
The descriptor regression checks that exactly these two bytes differ.
Enumeration and capture on actual Windows hardware remain to be checked.
Source: https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/pkey-audiodevice-enableendpointbydefault

Local dc6: 581564 bytes; SHA-256 `ae522103d256b528b688f53a534a14c74282f19f9bbaa133d7f6024df6b0700a`.
Target CPU1/flash audits, descriptor comparison and SMK USB driver tests pass.

## dc6 installed: capture category changed, host enablement pending

The owner installed dc6 and reconnected USB. Identity is
`MELODEE SMK 1.1.0-b1-dc6`; CPU1 is online with zero worker faults/rejects/
timeouts. Windows WDM-KS now names the four-channel input `Line (Melodee In)`
instead of `Synthesizer (Melodee In)`, confirming the descriptor change reached
the host. It still does not enumerate through WASAPI/MME. The existing Windows
capture endpoint retains registry state 0x10000001; no registry changes were
made. The owner has been asked to enable this endpoint through Sound Control
Panel. Capture success has not yet been established.

The recording tool now selects only a uniquely named four-channel Melodee
input, prefers WASAPI, supports both old/new endpoint labels, and refuses
WDM-KS-only or ambiguous inputs rather than recording a microphone. Four
selector tests pass. Its description now explicitly warns that firmware
autosave can persist live-sound changes even without a host SAVE command;
controlled chord tests must use the disposable test project.

## Capture endpoint enabled and missing SMK timer service found

The owner's Sound Control Panel change did not expose Melodee. The exact
Melodee capture endpoint was then enabled through Windows IPolicyConfig
SetEndpointVisibility(1), with its identity checked against the registry's
`Melodee In` property. No default audio device or other endpoint was changed.
Private journal: `build/dc6-windows-endpoint-enable.json`; DeviceState changed
from 0x10000001 to 0x1. WASAPI/MME now expose all four channels and accept
44.1 kHz PCM16. Windows retains the older friendly name `Synthesizer (Melodee
In)` while WDM-KS reflects the dc6 `Line` descriptor.

The ten-second controlled note/capture test returned zero host frames.
Firmware cap_alt was 1 and tx_packets remained 1 while cap_overruns increased.
Evidence: `build/dc6-capture-20261012-111158/result.json`. No USB capture pass.
Code inspection found that the SMK timer called usb_poll() but omitted
ua_service(), which upstream runs separately. SET_INTERFACE queued its first
packet but nothing serviced subsequent audio packets. dc7 restores this call
in the single existing USB-owner timer (2 kHz), keeping audio, MIDI and TRS
access serialized. No additional USB caller is introduced.

The timer body is shared with a host integration regression using the real
USB endpoint driver. Fifty simulated host frames are rearmed successfully,
MIDI/TRS remain serviced and a detached bus is untouched. Suppressing the
ua_service() call makes this regression fail. Additional main-loop native
preset/CZ/Prophet editing and storage helpers receive size optimization to
fit the restored audio path; the DSP kernels and flash layout are unchanged.
Windows IPolicyConfig ABI reference:
https://docs.rs/com-policy-config/latest/src/com_policy_config/lib.rs.html

Local dc7: 581532 bytes; SHA-256 `4426a29ad306a672464d778e44cb3a1238deae87d7b852dc86469f220987522c`.
Full host suite, native preset persistence tests and target CPU1/flash audits
pass. Physical USB capture with the restored timer service remains pending.

## dc7 physical capture and publication approval - 2026-10-12

The owner installed dc7 through normal USB. The ten-second analog probe captured
442,323 frames (10.03 s) with no host errors, capture underruns/overruns or new
missed render deadlines. The formerly stuck TX count reached 10,040.
Evidence: `build/dc7-capture-20261012-112243/result.json`.

The next test loaded the existing saved projects B and C, sent twenty four-note
chords per engine, recorded the exact Melodee four-channel WASAPI input at
44.1 kHz PCM16, and restored saved project D without editing any patches.

| Saved patch | Captured audio | CPU1 jobs | Worker faults / timeouts / rejects | USB capture under/overruns | Missed render deadlines | Voices shed |
| --- | --- | --- | --- | --- | --- | --- |
| FM6 | 60.6 s | 158,030 | 0 / 0 / 0 | 0 / 0 | 21 | 77 |
| Prophet | 60.8 s | 84,010 | 0 / 0 / 0 | 0 / 0 | 21 | 40 |

Neither recording reported a host capture error or bad USB packet. Channel 1
contained the selected track; the unused channels remained at the one-count
dither floor. These tests establish working capture and an exercised healthy
worker, not glitch-free synthesis: rendering still misses deadlines and the
overload guard sheds voices. WAV/JSON evidence stays private under
`build/dc7-core-capture-20261012-112331/`. These saved patches differ from the
prior INIT-preset comparison, so the numbers are not a controlled speedup study.

The owner approved public release with these issues documented. Follow-up:
reduce render deadline misses/voice shedding; validate longer USB recordings,
USB playback and PCM24; finish physical TRS expression/clock coverage, native
preset/favorites persistence and fader feel; test Elite on hardware. Keep all
model, calibration, boot-region and protected-write checks. No blanket
no-brick guarantee is inferred from the successful Pro updates.
