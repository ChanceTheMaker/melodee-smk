#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Stage the locally built development app; never copy vendor files or backups."""
import hashlib,json,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[1]
src=root/'build/melodee-smk-1.1.0-b1'
m=json.loads((src/'manifest.json').read_text())
name=m['image']
if Path(name).name!=name or not name.endswith('-candidate.bin'):raise SystemExit('Unexpected candidate filename')
data=(src/name).read_bytes()
if not m.get('installable') or not m.get('updater') or m.get('hardware_tested') or m.get('release_channel')!='local-development':raise SystemExit('Not a local development candidate')
if len(data)!=m['bytes'] or hashlib.sha256(data).hexdigest()!=m['sha256']:raise SystemExit('Candidate hash/size mismatch')
if data[:4]!=bytes.fromhex('04818000') or len(data)>0x8dfbc:raise SystemExit('Invalid application entry or size')
audit=(src/'storage-audit.json').read_bytes()
if hashlib.sha256(audit).hexdigest()!=m.get('storage_audit_sha256'):raise SystemExit('Storage audit hash mismatch')
a=json.loads(audit)
if a.get('host_tests_passed')!=['melodee_smk_storage_policy','melodee_smk_storage','melodee_smk_setup']:raise SystemExit('Storage tests not recorded')
if m.get('update_product')!='SMK-37_941' or m.get('storage_initialization')!='backup-required':raise SystemExit('Unexpected candidate storage/identity')
out=root/'web/firmware';out.mkdir(exist_ok=True)
shutil.copyfile(src/name,out/name)
shutil.copyfile(src/'storage-audit.json',out/'storage-audit.json')
(out/'candidate.json').write_text(json.dumps(m,indent=2)+'\n')
print('Staged '+m['firmware']+' — '+m['sha256'])
