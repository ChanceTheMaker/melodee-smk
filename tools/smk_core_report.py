# SPDX-License-Identifier: GPL-3.0-only
"""Summarize usb_audio_stats JSONL. No MIDI access or device writes."""
import argparse
import json
from pathlib import Path

COUNTERS = ('core1_jobs', 'core1_rejected', 'core1_faults', 'core1_timeouts',
            'audio_late', 'resource_failures', 'voices_shed', 'voices_given_up',
            'play_underruns', 'play_overruns', 'cap_underruns', 'cap_overruns',
            'bad_packets', 'missed_frames')

def summarize(rows):
    if len(rows) < 2:
        raise ValueError('At least two snapshots are required')
    first, last = rows[0], rows[-1]
    for previous, current in zip(rows, rows[1:]):
        if current['elapsed_s'] <= previous['elapsed_s']:
            raise ValueError('Snapshots must be from one ordered capture')
        for field in COUNTERS:
            if current[field] < previous[field]:
                raise ValueError('Counter reset or wrap during capture: ' + field)
    delta = {field: last[field] - first[field] for field in COUNTERS}
    errors = ('core1_rejected', 'core1_faults', 'core1_timeouts')
    healthy = all(row['core1_online'] == 1 and all(row[f] == 0 for f in errors) for row in rows)
    return {
        'samples': len(rows), 'duration_s': round(last['elapsed_s'] - first['elapsed_s'], 3),
        'worker_exercised': delta['core1_jobs'] > 0,
        'worker_healthy_at_all_samples': healthy,
        'counter_deltas': delta,
        'peak_sampled_voices': max(row['voices_active'] for row in rows),
        'sampled_cpu_pct_min': min(row['cpu_pct'] for row in rows),
        'sampled_cpu_pct_max': max(row['cpu_pct'] for row in rows),
        'max_job_us_since_boot': last['core1_max_job_us'],
        'max_wait_us_since_boot': last['core1_max_wait_us'],
        'usb_audio_stream_observed': any(row['play_alt'] or row['cap_alt'] for row in rows),
        'limitation': 'Polling diagnostics only; does not establish audible quality, USB capture quality, or a serial-versus-dual speedup.',
    }

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    args = parser.parse_args()
    print(json.dumps(summarize([json.loads(line) for line in args.capture.read_text().splitlines() if line.strip()]), indent=2))
