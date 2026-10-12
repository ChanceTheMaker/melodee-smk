# SPDX-License-Identifier: GPL-3.0-only
"""Fail-closed local-candidate gate for the SMK storage integration."""
import hashlib, os, re, shlex, subprocess

def audit_ram(symbols, disassembly):
    def symbol(name):
        rows=[line.split() for line in symbols.splitlines() if line.split() and line.split()[-1]==name]
        if len(rows)!=1: raise ValueError('Missing/ambiguous symbol: '+name)
        return rows[0]
    lo=int(symbol('_rt_start')[0],16);hi=int(symbol('_rt_end')[0],16)
    if lo!=0x1c00000 or not lo<hi<=0x1c08000: raise ValueError('RAM text bounds invalid')
    required=['fl_erase4k_ram','fl_prog_ram','fl_read_ram','fl_jedec_ram']
    for name in required:
        row=symbol(name);addr=int(row[0],16);size=int(row[-2],16)
        if '.ram_text' not in row or not size or not lo<=addr<addr+size<=hi:
            raise ValueError('Flash routine outside RAM text: '+name)
    instructions=0
    for line in disassembly.splitlines():
        match=re.match(r'^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2}\s+)+(.+)$',line)
        if not match: continue
        instructions+=1;address=int(match[1],16);asm=match[2]
        if not lo<=address<hi: raise ValueError('Unexpected RAM instruction address')
        # These routines are deliberately leaf functions: no libc, indirect calls,
        # or branches outside the copied RAM section while XIP is unavailable.
        if re.search(r'\b(?:call|icall)\b',asm): raise ValueError('Call in flash RAM routine: '+line)
        if 'goto' in asm:
            target=re.search(r':\s*([0-9a-f]+)\s*>',asm)
            if not target or not lo<=int(target[1],16)<hi: raise ValueError('Unbounded RAM branch: '+line)
    if instructions<100: raise ValueError('RAM disassembly missing or incomplete')
    return {'ram_start':hex(lo),'ram_end':hex(hi),'leaf_flash_routines':required,'instructions_checked':instructions}

def audit_storage(root,out,symbols,disassembly):
    report=audit_ram(symbols,disassembly)
    host=out/'storage-checks';host.mkdir(exist_ok=True)
    tests=['melodee_smk_storage_policy','melodee_smk_storage','melodee_smk_setup']
    for name in tests:
        exe=host/name
        subprocess.run([*shlex.split(os.environ.get('HOST_CC','cc')),'-O2','-std=c11',str(root/'tests'/(name+'_test.c')),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
    report['host_tests_passed']=tests
    report['ram_disassembly_sha256']=hashlib.sha256(disassembly.encode()).hexdigest()
    report['write_ranges']=[['0x9d000','0xc0000'],['0xc2000','0xff000']]
    report['initialization']='explicit verified-backup gate; reboot required'
    return report
