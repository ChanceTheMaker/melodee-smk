# SPDX-License-Identifier: GPL-3.0-only
"""Owner-approved Melodee save trial. Prepare uses only empty slots; verify never saves."""
import argparse
import json
from pathlib import Path
import time
import mido
from usb_audio_stats import resolve_port,snapshot
from smk_storage_backup import request as storage_request
HDR=[125,70,76]
def req(ip,op,cmd,args=()):
    op.send(mido.Message('sysex',data=HDR+[cmd]+list(args)))
    end=time.monotonic()+30
    while time.monotonic()<end:
        for msg in ip.iter_pending():
            if msg.type=='sysex' and list(msg.data[:4])==HDR+[cmd]: return list(msg.data[4:])
        time.sleep(.004)
    raise TimeoutError('No reply to command '+str(cmd)+'; do not repeat a save blindly')
def main():
    p=argparse.ArgumentParser(description=__doc__)
    mode=p.add_mutually_exclusive_group(required=True)
    mode.add_argument('--prepare',action='store_true');mode.add_argument('--verify',action='store_true')
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--expected-version',default='MELODEE SMK 1.1.0-b1-dc3');a=p.parse_args()
    if a.prepare:a.output.mkdir(parents=True,exist_ok=False)
    with mido.open_input(resolve_port('Melodee',mido.get_input_names())) as ip,mido.open_output(resolve_port('Melodee',mido.get_output_names())) as op:
        info=req(ip,op,1);end=info.index(0);version=bytes(info[:end]).decode();np=info[end+2]
        if version!=a.expected_version:raise RuntimeError('Unexpected firmware')
        cap=storage_request(ip,op,0)
        if len(cap)!=12 or cap[:2]!=[1,1]:raise RuntimeError('Storage not mounted')
        stats=snapshot(ip,op,voices=True,cores=True,memory=True)
        if stats['voices_active'] or stats['voices_held']:raise RuntimeError('Release keys/pedal before save test')
        def sound():return req(ip,op,4)[:2+2*np]
        if a.prepare:
            for slot in range(4):
                if req(ip,op,9,[2,slot])!=[2,slot,0]:raise RuntimeError('Project slot occupied; no saves started')
            if req(ip,op,17,[0])[1]:raise RuntimeError('User preset U1 occupied; no saves started')
            expected={'version':version,'projects':[],'before_stats':stats}
            (a.output/'expected.json').write_text(json.dumps(expected,indent=2))
            req(ip,op,27,[0])
            for slot,(engine,preset) in enumerate(((0,1),(12,1),(19,1),(0,2))):
                if req(ip,op,9,[2,slot])!=[2,slot,0]:raise RuntimeError('Slot changed; refusing overwrite')
                chosen=req(ip,op,8,[engine,preset,0]);time.sleep(.15)
                if chosen!=[engine,preset,0]:raise RuntimeError('Preset selection mismatch')
                # Distinct first sequencer note in each project; no playback.
                req(ip,op,7,[0,1,60+slot,0,0,0,0,0,90])
                row={'slot':slot,'sound':sound(),'step':req(ip,op,6,[0])}
                if slot==0:
                    if req(ip,op,19,[0]+list(b'DC3 SAVE A'))!=[0,0]:raise RuntimeError('User preset save failed')
                    expected['user_preset']=req(ip,op,17,[0])
                expected['projects'].append(row)
                (a.output/'expected.json').write_text(json.dumps(expected,indent=2))
                if req(ip,op,9,[1,slot])!=[1,slot,1]:raise RuntimeError('Project save not acknowledged')
                print('Project',chr(65+slot),'saved',flush=True)
        else:expected=json.loads((a.output/'expected.json').read_text())
        if len(expected['projects'])!=4:raise RuntimeError('Incomplete save trial')
        for row in expected['projects']:
            slot=row['slot']
            if req(ip,op,9,[2,slot])!=[2,slot,1]:raise RuntimeError('Saved project missing: '+str(slot))
            if req(ip,op,9,[0,slot])!=[0,slot,1]:raise RuntimeError('Load failed')
            time.sleep(.15)
            if sound()!=row['sound']:raise RuntimeError('Project sound mismatch: '+str(slot))
            if req(ip,op,6,[0])!=row['step']:raise RuntimeError('Project step mismatch: '+str(slot))
            print('Project',chr(65+slot),'sound and step match',flush=True)
        if req(ip,op,17,[0])!=expected['user_preset']:raise RuntimeError('User preset bytes changed')
        if req(ip,op,20,[0])!=[0,0]:raise RuntimeError('User preset load failed')
        # Loading a user sound edits the current project. Restore the saved
        # project before the upstream five-second idle autosave can write it.
        if req(ip,op,9,[0,3])!=[0,3,1]:raise RuntimeError('Final project restore failed')
        final=snapshot(ip,op,voices=True,cores=True,memory=True)
        (a.output/('immediate-result.json' if a.prepare else 'restart-result.json')).write_text(json.dumps(final,indent=2))
        print('User preset U1 intact and loaded; CPU1 online='+str(final['core1_online']),flush=True)
if __name__=='__main__':main()
