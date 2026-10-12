# SPDX-License-Identifier: GPL-3.0-only
"""Read the fixed Melodee SMK music area twice. Never initializes or writes flash."""
import argparse
import hashlib
import json
from pathlib import Path
import time
import zlib
import mido
from usb_audio_stats import resolve_port
PREFIX = [125,77,69,76,1]
SIZE = 393216

def u32(n): return [(n >> (7*i)) & 127 for i in range(5)]
def read32(data):
    if len(data) != 5 or data[4] > 15: raise ValueError('Invalid 32-bit field')
    return sum(v << (7*i) for i,v in enumerate(data))
def unpack(data, count):
    out = bytearray()
    while data:
        mask, data = data[0], data[1:]
        k = min(7, len(data))
        if not k or mask >> k: raise ValueError('Malformed packed data')
        out.extend(v | (((mask >> i) & 1) << 7) for i,v in enumerate(data[:k]))
        data = data[k:]
    if len(out) != count: raise ValueError('Short backup data')
    return out

def request(incoming, outgoing, op, args=()):
    if op not in (0,1): raise ValueError('This tool supports read-only operations only')
    outgoing.send(mido.Message('sysex', data=PREFIX+[op]+list(args)))
    end = time.monotonic()+5
    while time.monotonic() < end:
        for msg in incoming.iter_pending():
            p=list(msg.data) if msg.type=='sysex' else []
            if p[:6] != PREFIX+[op]: continue
            if len(p)<7: raise ValueError('Short reply')
            if p[6]: raise RuntimeError('Device refused read: '+str(p[6]))
            result=p[7:]
            if op==1 and result[:5]!=list(args[:5]): continue
            return result
        time.sleep(.002)
    raise TimeoutError('Storage read timed out at '+str(read32(list(args[:5])) if op==1 else 'status')+'; no writes were sent')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args=parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    with mido.open_input(resolve_port('Melodee',mido.get_input_names())) as incoming, mido.open_output(resolve_port('Melodee',mido.get_output_names())) as outgoing:
        cap=request(incoming,outgoing,0)
        if len(cap)!=12 or cap[0]!=1 or read32(cap[7:12])!=SIZE: raise RuntimeError('Unexpected storage layout')
        report={'unit':read32(cap[2:7]),'mounted':bool(cap[1]),'bytes':SIZE,'passes':[],'writes_sent':False,'read_retries':0}
        first=None
        for pass_index in range(2):
            data=bytearray()
            for off in range(0,SIZE,256):
                for attempt in range(3):
                    try:
                        p=request(incoming,outgoing,1,u32(off)+[0,2])
                        break
                    except TimeoutError:
                        if attempt==2: raise
                        report['read_retries']+=1
                        print('Retrying read-only chunk',off,flush=True)
                if len(p)<12 or read32(p[:5])!=off or p[5]+(p[6]<<7)!=256: raise ValueError('Offset/length mismatch')
                chunk=unpack(p[12:],256)
                if zlib.crc32(chunk)!=read32(p[7:12]): raise ValueError('Chunk CRC mismatch')
                data.extend(chunk)
                time.sleep(.02)  # Let the device finish its USB transmit queue before the next request.
                if off%65536==0: print('Backup pass',pass_index+1,round(off*100/SIZE),'%',flush=True)
            (args.output/('pass-'+str(pass_index+1)+'.bin')).write_bytes(data)
            report['passes'].append({'sha256':hashlib.sha256(data).hexdigest(),'crc32':zlib.crc32(data)})
            if first is None: first=data
            elif data!=first: raise RuntimeError('Backup reads differ; do not initialize')
        report['matching']=True
        report['device_initialization_gate_ready']=report['read_retries']==0
        (args.output/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
        print('Two complete backups match. No flash writes sent.',flush=True)

if __name__=='__main__': main()
