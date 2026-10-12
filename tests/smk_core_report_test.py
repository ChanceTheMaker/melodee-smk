# SPDX-License-Identifier: GPL-3.0-only
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from smk_core_report import COUNTERS, summarize

def sample(t, jobs):
    return {**dict.fromkeys(COUNTERS, 0), 'elapsed_s': t, 'core1_jobs': jobs,
            'audio_late': 1, 'core1_online': 1, 'voices_active': 4, 'cpu_pct': 40,
            'core1_max_job_us': 50, 'core1_max_wait_us': 20, 'play_alt': 0, 'cap_alt': 0}

class ReportTest(unittest.TestCase):
    def test_healthy(self):
        r = summarize([sample(0, 0), sample(5, 200)])
        self.assertTrue(r['worker_exercised'])
        self.assertTrue(r['worker_healthy_at_all_samples'])
        self.assertEqual(r['counter_deltas']['audio_late'], 0)
        self.assertFalse(r['usb_audio_stream_observed'])

    def test_retirement_and_idle(self):
        rows = [sample(0, 0), {**sample(5, 0), 'core1_online': 0, 'core1_faults': 1}]
        r = summarize(rows)
        self.assertFalse(r['worker_healthy_at_all_samples'])
        self.assertFalse(r['worker_exercised'])

    def test_reset_and_order(self):
        for rows in ([sample(0, 100), sample(5, 0)], [sample(5, 0), sample(0, 0)], [sample(0, 0)]):
            with self.assertRaises(ValueError): summarize(rows)

if __name__ == '__main__': unittest.main()
