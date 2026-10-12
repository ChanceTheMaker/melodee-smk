# SPDX-License-Identifier: GPL-3.0-only
"""Prepare a verified-storage initialization plan; --execute requires explicit owner approval."""
import argparse
import hashlib
import json
from pathlib import Path
import time
import zlib
import mido
from smk_storage_backup import PREFIX, SIZE, read32, request, u32
from usb_audio_stats import resolve_port, snapshot

def plan(folder):
    meta=json.loads((folder/'manifest.json').read_text())
    if not meta.get('matching') or not meta.get('device_initialization_gate_ready') or meta.get('read_retries')!=0:
        raise ValueError('An uninterrupted matching backup is required')
    a=(folder/'pass-1.bin').read_bytes();b=(folder/'pass-2.bin').read_bytes()
    if a!=b or len(a)!=SIZE or meta['bytes']!=SIZE: raise ValueError('Backup copies differ or have wrong size')
    digest=hashlib.sha256(a).hexdigest();crc=zlib.crc32(a)
    if len(meta['passes'])!=2 or any(p['sha256']!=digest or p['crc32']!=crc for p in meta['passes']):
        raise ValueError('Backup checksum mismatch')
    return {'unit':meta['unit'],'bytes':SIZE,'crc32':crc,'sha256':digest,
            'action':'Replace the backed-up saved music/sample area with empty Melodee storage',
            'protected':'Application, bootloader and factory calibration are outside these ranges',
            'ranges':['0x9d000..0xbffff','0xc2000..0xfefff']}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--backup',required=True,type=Path)
    parser.add_argument('--execute',action='store_true',help='Only after owner approval to erase the backed-up music/sample area')
    parser.add_argument('--journal',type=Path,help='New private execution journal, required with --execute')
    args=parser.parse_args();p=plan(args.backup)
    print(json.dumps(p,indent=2),flush=True)
    if not args.execute: return
    if args.journal is None: parser.error('--journal is required with --execute')
    if time.time()-(args.backup/'manifest.json').stat().st_mtime>1500: raise RuntimeError('Backup is too old; obtain a fresh one')
    with args.journal.open('x') as log:
        def record(item): log.write(json.dumps(item)+'\n');log.flush();print(json.dumps(item),flush=True)
        with mido.open_input(resolve_port('Melodee',mido.get_input_names())) as ip, mido.open_output(resolve_port('Melodee',mido.get_output_names())) as op:
            cap=request(ip,op,0)
            if len(cap)!=12 or cap[0]!=1 or cap[1]!=0 or read32(cap[2:7])!=p['unit'] or read32(cap[7:12])!=SIZE:
                raise RuntimeError('Device/layout changed or already initialized; no erase sent')
            stats=snapshot(ip,op,voices=True,cores=True,memory=True)
            if stats['voices_active'] or stats['voices_held'] or stats['play_alt']:
                raise RuntimeError('Release keys/pedal and stop audio before setup')
            record({'state':'sending-one-initialize-request','plan':p})
            op.send(mido.Message('sysex',data=PREFIX+[2]+u32(p['crc32'])+u32(p['unit'])+list(b'ERASE')))
            end=time.monotonic()+180
            while time.monotonic()<end:
                for msg in ip.iter_pending():
                    data=list(msg.data) if msg.type=='sysex' else []
                    if data[:6]!=PREFIX+[2]: continue
                    if len(data)!=7: raise RuntimeError('Malformed initialization reply; do not retry')
                    record({'state':'device-reply','code':data[6]})
                    if data[6]: raise RuntimeError('Device refused or failed initialization; inspect code before proceeding')
                    record({'state':'initialized-restart-required'})
                    return
                time.sleep(.02)
            record({'state':'no-reply-do-not-retry'})
            raise TimeoutError('No completion reply; inspect device without repeating erase')

if __name__=='__main__': main()
