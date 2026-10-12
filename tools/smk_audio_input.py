# SPDX-License-Identifier: GPL-3.0-only
"""Select only Melodee capture; never fall back to a system microphone."""
def select_input(devices, hosts):
    preferred = {'Windows WASAPI': 0, 'Core Audio': 0, 'ALSA': 0,
                 'MME': 1, 'Windows DirectSound': 2}
    matches = []
    for index, device in enumerate(devices):
        host = hosts[device['hostapi']]['name']
        if (device['name'] in ('Line (Melodee In)', 'Synthesizer (Melodee In)', 'Melodee In')
                and device['max_input_channels'] == 4 and host in preferred):
            matches.append((preferred[host], index, device))
    if not matches:
        raise RuntimeError('Enable the four-channel Melodee In recording endpoint in Windows Sound first; no other input will be recorded')
    best = min(row[0] for row in matches)
    choices = [row for row in matches if row[0] == best]
    if len(choices) != 1:
        raise RuntimeError('Multiple Melodee recording inputs; connect only the test unit')
    return choices[0][1], choices[0][2]
