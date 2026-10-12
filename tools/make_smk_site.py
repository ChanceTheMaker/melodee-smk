#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Stage the website and the explicitly approved experimental b1 hardware beta."""
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
                            source.name in ('index.html', 'editor.html', 'simulator.html', 'storage.html')):
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
    'image': 'melodee-smk-1.1.0-b1-candidate.bin',
    'sha256': 'b03c3dfee7966e43e5f542a6c17c7f79e34df14283d8db83229fe3cc4abce40a',
    'bytes': 581252,
    'update_product': 'SMK-37_941',
    'update_version': 'MELODEE SMK 1.1.0-b1',
    'release_channel': 'public-beta',
    'installable': True,
    'updater': True,
    'hardware_tested': False,
    'persistent_storage': True,
    'usb_audio': True,
    'storage_initialization': 'backup-required',
    'storage_audit_sha256': '42989eb46f33be9524527d382b2c40e8c3bcce03836b1a46505f5189bb545209',
}
for key, value in expected.items():
    if manifest.get(key) != value:
        raise SystemExit('Unapproved firmware metadata: ' + key)
app = (firmware / expected['image']).read_bytes()
if len(app) != expected['bytes'] or hashlib.sha256(app).hexdigest() != expected['sha256']:
    raise SystemExit('Approved application hash/size mismatch')
audit = (firmware / 'storage-audit.json').read_bytes()
if hashlib.sha256(audit).hexdigest() != expected['storage_audit_sha256']:
    raise SystemExit('Approved storage audit hash mismatch')
shutil.copy2(web / 'installer.html', out / 'installer.html')
(out / 'installer').mkdir()
for source in (web / 'installer').iterdir():
    if source.suffix in ('.js', '.css'):
        shutil.copy2(source, out / 'installer' / source.name)
(out / 'firmware').mkdir()
for name in ('candidate.json', expected['image'], 'storage-audit.json'):
    shutil.copy2(firmware / name, out / 'firmware' / name)
(out / '.nojekyll').write_text('')
print('Staged website and experimental development candidate:', out)
