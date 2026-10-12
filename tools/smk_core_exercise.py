# SPDX-License-Identifier: GPL-3.0-only
"""Controlled Melodee SMK chord/USB capture trial. Changes live sounds; firmware autosave can persist them. Never flashes."""
import argparse
import json
import queue
import time
import wave
from pathlib import Path
import mido
from usb_audio_stats import resolve_port, snapshot
from smk_audio_input import select_input

HEADER = [0x7d, 0x46, 0x4c]
NOTES = (60, 64, 67, 71)

def request(incoming, outgoing, command, args=()):
    outgoing.send(mido.Message('sysex', data=HEADER + [command] + list(args)))
    deadline = time.monotonic() + 3
    while time.monotonic() < deadline:
        for message in incoming.iter_pending():
            if message.type == 'sysex' and list(message.data[:4]) == HEADER + [command]:
                return list(message.data[4:])
        time.sleep(.005)
    raise TimeoutError('No editor reply for command ' + str(command))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--run', action='store_true', required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cores', type=int, choices=(0, 1), required=True)
    parser.add_argument('--rounds', type=int, default=20)
    parser.add_argument('--notes', type=int, nargs='+', default=NOTES, help='Distinct MIDI notes, at most eight')
    parser.add_argument('--capture', action='store_true', help='Record only the uniquely named Melodee In USB input')
    args = parser.parse_args()
    if not 1 <= args.rounds <= 100: parser.error('rounds must be 1..100')
    if not 1 <= len(args.notes) <= 8 or len(set(args.notes)) != len(args.notes) or any(n < 0 or n > 127 for n in args.notes):
        parser.error('notes must contain 1..8 distinct MIDI notes in 0..127')
    args.output.mkdir(parents=True, exist_ok=False)
    ip = resolve_port('Melodee', mido.get_input_names())
    op = resolve_port('Melodee', mido.get_output_names())
    with mido.open_input(ip) as incoming, mido.open_output(op) as outgoing:
        before = {'info': request(incoming, outgoing, 1), 'track': request(incoming, outgoing, 27),
                  'dump': request(incoming, outgoing, 4)}
        (args.output / 'before.json').write_text(json.dumps(before, indent=2))
        first = snapshot(incoming, outgoing, voices=True, cores=True, memory=True)
        if first['core1_online'] != args.cores: raise RuntimeError('Wrong core configuration; no notes sent')
        if first['voices_held']: raise RuntimeError('Release keys and sustain before the controlled trial')
        request(incoming, outgoing, 27, [0])
        try:
            for engine, name in ((12, 'fm6'), (19, 'prophet')):
                reply = request(incoming, outgoing, 8, [engine, 0, 0])
                if reply[:2] != [engine, 0]: raise RuntimeError('Preset selection failed')
                outgoing.send(mido.Message('control_change', channel=0, control=64, value=0))
                outgoing.send(mido.Message('control_change', channel=0, control=1, value=0))
                outgoing.send(mido.Message('pitchwheel', channel=0, pitch=0))
                time.sleep(1)
                stream = None
                audio = queue.SimpleQueue()
                capture = {'requested': args.capture, 'errors': [], 'frames': 0}
                wav = None
                try:
                    if args.capture:
                        import sounddevice as sd
                        index, device = select_input(sd.query_devices(), sd.query_hostapis())
                        capture['device'] = dict(device)
                        def callback(data, frames, timing, status):
                            audio.put((bytes(data), str(status) if status else ''))
                        stream = sd.RawInputStream(device=index, channels=4, samplerate=44100,
                                                   dtype='int16', blocksize=4410, latency='high', callback=callback)
                        wav = wave.open(str(args.output / (name + '.wav')), 'wb')
                        wav.setparams((4, 2, 44100, 0, 'NONE', 'not compressed'))
                        stream.start()
                except Exception as error:
                    capture['errors'].append(str(error))
                    if stream: stream.close()
                    stream = None
                def drain():
                    while not audio.empty():
                        data, error = audio.get()
                        if wav: wav.writeframesraw(data)
                        capture['frames'] += len(data) // 8
                        if error: capture['errors'].append(error)
                start = time.monotonic()
                try:
                    with (args.output / (name + '.jsonl')).open('x') as log:
                        def record(phase):
                            s = snapshot(incoming, outgoing, voices=True, cores=True, memory=True)
                            s.update(elapsed_s=round(time.monotonic()-start, 3), cpu_pct=round(s['cpu_q8']*100/256, 1), phase=phase)
                            log.write(json.dumps(s)+'\n');log.flush();drain()
                            if s['core1_online'] != args.cores: raise RuntimeError('Worker retired during trial')
                        record('baseline')
                        print(name + ': controlled ' + str(len(args.notes)) + '-note chords started', flush=True)
                        for iteration in range(args.rounds):
                            for note in args.notes: outgoing.send(mido.Message('note_on', channel=0, note=note, velocity=90))
                            for second in range(2): time.sleep(1);record('held')
                            for note in args.notes: outgoing.send(mido.Message('note_off', channel=0, note=note))
                            time.sleep(1);record('released')
                        print(name + ': completed', flush=True)
                finally:
                    if stream: stream.stop();stream.close()
                    drain()
                    if wav: wav.close()
                    (args.output / (name + '-capture.json')).write_text(json.dumps(capture, indent=2))
        finally:
            for note in args.notes: outgoing.send(mido.Message('note_off', channel=0, note=note))
            outgoing.send(mido.Message('control_change', channel=0, control=64, value=0))
            outgoing.send(mido.Message('pitchwheel', channel=0, pitch=0))
            outgoing.send(mido.Message('control_change', channel=0, control=1, value=0))

if __name__ == '__main__': main()
