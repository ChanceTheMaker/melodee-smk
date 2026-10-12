#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Stage the website and the explicitly approved experimental hardware candidate."""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('out', type=Path)
out = p.parse_args().out.resolve()
if out.exists():
    p.error('Output must be a new directory.')
out.mkdir(parents=True)
web = ROOT / 'web'
for source in web.iterdir():
    if source.is_file() and (source.suffix in ('.js', '.css', '.svg', '.png', '.ttf', '.txt') or
                            source.name in ('index.html', 'editor.html', 'simulator.html')):
        shutil.copy2(source, out / source.name)
for name in ('audio', 'fonts', 'screenshots'):
    shutil.copytree(web / name, out / name)
for name in ('LICENSE', 'LICENSING.md'):
    shutil.copy2(ROOT / name, out / name)
shutil.copytree(ROOT / 'LICENSES', out / 'LICENSES')
# Pin the approved artifact: a local rebuild must not silently become public.
firmware = web / 'firmware'
manifest = json.loads((firmware / 'candidate.json').read_text(encoding='utf-8'))
expected = {
    'image': 'melodee-smk-1.1.0-dev1-candidate.bin',
    'sha256': 'f6bbd1b5a1470accb9b9b87b03daa742ed59d01edb597333c79980140c36fa4d',
    'bytes': 578624,
    'update_product': 'SMK-37_940',
    'update_version': 'MELODEE SMK 1.1.0-dev1',
    'release_channel': 'public-development',
    'installable': True,
    'updater': True,
    'hardware_tested': False,
    'persistent_storage': False,
    'usb_audio': False,
}
for key, value in expected.items():
    if manifest.get(key) != value:
        raise SystemExit('Unapproved firmware metadata: ' + key)
app = (firmware / expected['image']).read_bytes()
if len(app) != expected['bytes'] or hashlib.sha256(app).hexdigest() != expected['sha256']:
    raise SystemExit('Approved application hash/size mismatch')
shutil.copy2(web / 'installer.html', out / 'installer.html')
(out / 'installer').mkdir()
for source in (web / 'installer').iterdir():
    if source.suffix in ('.js', '.css'):
        shutil.copy2(source, out / 'installer' / source.name)
(out / 'firmware').mkdir()
for name in ('candidate.json', expected['image']):
    shutil.copy2(firmware / name, out / 'firmware' / name)
(out / '.nojekyll').write_text('')
print('Staged website and experimental development candidate:', out)
