# Melodee SMK

Independent SMK-37 Pro & Elite port of [Melodee](https://github.com/keremimo/melodee), with the shared SMK Studio and simulator website.

**Current work: 1.1.0-b1 — local integration and device-test preparation.**

The local installer now stages b1 after passing its RAM-placement, protected-range, interrupted-write and backup-gate checks. See [the b1 test and storage guide](docs/SMK_BETA_TEST.md) for the exact release state and device checks.

The user has installed and played the earlier dev1 build on an SMK. The new b1 persistence, physical MIDI and USB audio changes still require device validation. The browser uses Melodee's C synthesis and UI compiled to WebAssembly; it is not a cycle-accurate emulator of the JieLi chip, USB hardware or Bluetooth radio.

## Open locally

Serve the web directory on localhost (Web Audio and Web MIDI require a secure context):

```sh
python -m http.server 8791 --bind 127.0.0.1 --directory web
```

- Front page: http://localhost:8791/
- Browser Studio: http://localhost:8791/editor.html?audio=1
- SMK simulator: http://localhost:8791/simulator.html
- Development installer: http://localhost:8791/installer.html

The front page includes the rotating Pro/Elite hardware illustration, native Melodee screen, feature overview, guide and links to both browser views. The separate development installer prepares the staged candidate for either model. It does not open MIDI ports until Install is explicitly clicked.

## Studio and simulator

- Native Melodee engines: Prophet, FM6, CZ-1, SID and drums; native editor protocol and sound parameter descriptors.
- Melodee's existing deep Prophet/CZ/FM6 editors, sequencer, track mixer, browser Library, project and settings screens.
- Shared website themes: Melodee Atelier, Stage Red, Matrix, Vintage DX7, Model D Walnut, Chocolate Factory, Vaporwave, Midnight Studio, Space Mission, Bauhaus, Ocean Lab, Arcade '84 and high contrast; light/dark modes and full-width toggle.
- Knobs/sliders per Sound card, movable cards, native parameter readouts and interactive envelope/LFO/arpeggiator/effect guides.
- Fixed browser keyboard with a collapsible tray, sustain, velocity and track routing; live oscilloscope.
- SMK simulator with 37 keys, 16 pads, 8 knobs, 4 faders, pitch/mod wheels, sustain, native LCD, Pro/Elite finish swatches, compact/expanded view, fullscreen and replayable tutorial.
- USB MIDI controller input in both browser views, requested only through Enable USB MIDI input. Performance input does not request SysEx or open a MIDI output.
- Browser audio prepares automatically in Simulator; browser autoplay restrictions may require a key/pad click or Enable audio. Studio starts through Browser Studio. Release notes handles held notes and sustain.

Browser Studio and Simulator share a complete session for the exact compiled runtime version: current music, all 32 pattern banks, projects A–D, general and native FM6/CZ/Prophet presets, favorites, settings and the startup template. Sessions autosave after edits and can be exported/imported as local JSON. Imports validate the entire container and native banks before replacing music; invalid imports leave the current music intact. The original browser Library and website appearance have their own persistent storage. Imported files stay local.

Session schema 2 is tied to the compiled runtime SHA. Older schema-1 current-song exports and exports from another runtime are not imported automatically; retain them separately. Browser storage can be cleared by the browser, so export a backup of important work. Samples and transient playback/held-key state are not part of this session.

## Panel mapping

| Hardware / simulator control | Mapping |
| --- | --- |
| Knob bank 1 | Master volume, Tempo/SELECT, Presets, Track/ALGORITHM, K1–K4 |
| Knob bank 2 | Attack, Decay, Sustain, Release, Distortion, Chorus, Delay, Reverb |
| Faders, bank 1 | Four track levels |
| Faders, bank 2 | Selected track attack/decay/sustain/release |
| PATCH / PARA / BT | Native Home / Save / Edit |
| ARP / NOTE REPEAT / SCALE / CHORD | Native ARP / LFO / Scale / Envelope pages |
| GLO / FX / SEQ / PLAY / REC | Matching native page/action |
| Piano | C3–C6, octave shift; scale/chord processing and sustain |
| Pads | Drum-track MIDI notes using the shared SMK pad map |

Native FM-1 key-layer shortcuts occupy the inner F3–G5 range; the full 37-key UI adaptation still needs a hardware usability pass. Native engine-specific envelope editing remains available in Studio; alternate bank ADSR knobs currently address the common track envelope parameters.

## Build and tests

Source baseline: keremimo/melodee commit **01a78bd** (1.1.0). Integration branch: **feat/smk-hardware-beta**.

Generate native fonts/DSP tables with the upstream tools. Pillow must include Raqm; fonttools is also required. Zig 0.13.0 is the tested browser compiler:

```sh
python tools/build_melodee_smk_browser.py --zig /path/to/zig --generate
node tests/smk_browser_test.mjs
```

The browser builder writes web/audio/smk-engine.wasm and its SHA-256 manifest. Already generated tables can be reused by omitting --generate.

For the hardware development image, use the upstream Linux JieLi toolchain (native Linux or WSL), set JIELI_TOOLCHAIN to its directory, and run:

```sh
JIELI_DOCKER=0 python tools/build_melodee_smk.py
```

Output is under build/melodee-smk-1.1.0-b1/. The candidate binary embeds an 8,160-byte RAM-only updater and advertises SMK-37_941. The manifest records memory bounds, SHA-256, capabilities, storage-audit hash and hardware-test status. Stage it for the website with `python tools/stage_melodee_smk.py`; do not feed the raw .bin to a vendor updater. The website packages it with a hash-checked official Pro/Elite 016 base entirely in the browser.

Automated runtime coverage: all five main engines produce audio; editor INFO, DUMP and all parameter descriptors; all 37 keys and 16 pad feedback bits; sustain/release; fresh-runtime restoration of projects A–D, all 32 patterns, native preset banks, general presets, favorites, settings and song chains; corrupt snapshot rejection without changing current music; read-only polling does not postpone autosave. Desktop/mobile browser checks cover pages, native editor connection and control rendering.

## Development installer

The local page automatically fetches the staged GPL application and an immutable mirror of the official model-specific 016 package. Both complete stock SHA-256 and application SHA-256/size are checked. A local official .fwsc file is an optional fallback; files are not uploaded. Model changes discard the prepared candidate and reset confirmation.

Automatic install/resume detects a portable SMK updater (identity 922 or newer), exact stock Pro/Elite 016, or its active loader. Wrong stock models/versions and older private updaters are refused. An ambiguous Elite loader requires explicit device confirmation; it is never automatically treated as an SMK.

The embedded RAM loader validates JEDEC ID, supported boot-region checksum and the unit's calibration structure. It preflights the complete payload before erasing, rechecks each fetched sector, programs changed sectors only, reads them back, and checks the protected boot/calibration regions afterward. Writes are constrained to 0x4000–0x9d000 (end exclusive). This does not guarantee recovery from lost power while the application is being written.

**Before a physical test:** export important music from the installed firmware. b1 integrates guarded device saves, TRS output and 44.1 kHz USB audio. Sample upload and Bluetooth remain unavailable. First-time storage initialization is separate, requires a verified backup, and replaces old saved music/sample data in its designated regions. Retaining bytes outside the application region does not make another firmware's saves compatible with Melodee.

Offline verification (no MIDI device opened):

```sh
node web/installer/test_smk37_public.mjs /private/pro-016.fwsc /private/elite-016.fwsc build/melodee-smk-1.1.0-b1/manifest.json
cc -std=c11 -O2 -Ifirmware/hal -DTEST_PUBLIC tests/smk37_update_test.c -o build/smk37-update-test
build/smk37-update-test
build/smk37-update-test build/public-tests/pro-synthetic-flash.bin build/public-tests/pro.smkupdate
build/smk37-update-test build/public-tests/elite-synthetic-flash.bin build/public-tests/elite.smkupdate
```

Verified: both stock container/CRC round trips, protected bytes unchanged, corrupt files rejected; exact stock and custom-updater selection, missing loader, programming failure, wrong returned identity and cleanup; simulated flash preflight, changed host data, interruption/retry, no-op and calibration preservation across synthetic units. The real Melodee candidate passes the RAM-loader test for both model packages. Browser checks verify automatic preparation and checkbox gating without pressing Install.

## Hardware integration status

The b1 source adds SMK-specific transactional storage, backup-gated initialization, TRS MIDI output for notes/expression and internal clock/Start/Stop, native 44.1 kHz USB audio, pad brightness and LCD sleep/wake. These features have host tests but still need the [device validation checklist](docs/SMK_BETA_TEST.md). The local installer serves b1 for those device checks; the public release is unchanged.

The target remains single-core with cache RAM disabled. FM-1 input/audio/ADC initialization and OTA staging remain excluded. Bluetooth, user sample upload, TRS Continue/external-clock forwarding and broader long-run hardware validation remain separate work. Private device backups and vendor binaries are not included.

## Provenance

Melodee is by Kerem Kilic / Ellic Studio, derived from Felucca by Leo Kuroshita / Hügelton Instruments. The SMK hardware adapters and website components are adapted from ChanceTheMaker/sloop-smk revision **090588d**; their original GPL notices are retained. Port and integration work: Chance Roth (@ChanceTheMaker).

See LICENSE, LICENSING.md, web/FUKIAI-LICENSE.txt and bundled font licenses. The original upstream README below remains an FM-1 reference; its installation instructions do not apply to this SMK development build.
