#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import re,json,hashlib
import build

def build_loader(out,gen,product="SMK-37_911"):
    src=build.FW/'loader'; flags=[*build.CFLAGS,'-Ifirmware/hal',f'-I{gen.relative_to(build.SRC)}']
    build.tc_all(('cc','-c',src/'crt0_ldr.S','-o',out/'ldr-start.o'),
                 ('cc',*flags,'-c',src/'smk37_loader.c','-o',out/'ldr.o'))
    elf=out/'smk37-loader.elf'
    build.tc('pi32v2/bin/ld','--gc-sections','-e','_start','-T',src/'loader.ld',out/'ldr-start.o',out/'ldr.o','-o',elf)
    build.tc('common/bin/objcopy','-O','binary','-j','.text',elf,out/'smk37-loader.bin')
    syms=build.tc('common/bin/objdump','-t',elf); sections=build.tc('common/bin/objdump','-h',elf)
    dis=build.tc('common/bin/objdump','-d',elf); (out/'smk37-loader.dis').write_text(dis)
    for name in ['_start','_bss_end','fl_read_ram','fl_prog_ram','fl_erase4k_ram','smku_session']:
        match=re.search(r'^([0-9a-f]+) .*\s'+name+r'$',syms,re.M)
        if not match or not 0x1c0a800<=int(match[1],16)<0x1c1e800:raise SystemExit('Loader RAM symbol invalid: '+name)
    for line in sections.splitlines():
        p=line.split()
        if len(p)>5 and p[0].isdigit() and int(p[2],16) and p[1] in ('.text','.bss','.data','.ram_text'):
            a=int(p[3],16);n=int(p[2],16)
            if not 0x1c0a800<=a<=a+n<=0x1c1e800:raise SystemExit('Loader section outside RAM')
    if re.search(r'\b(?:ota_session|ldr_session|ota_commit|ota_stage|ldr_record_clear)\b',syms):raise SystemExit('FM-1 updater staging linked')
    data=(out/'smk37-loader.bin').read_bytes()
    if len(data)>0x14000:raise SystemExit('Loader too large')
    body=product.encode('ascii').ljust(27,b'\0'); msg=bytes([0,0x59,0x11,27,0,0])+body+bytes([~sum(body)&255])
    packed=[];acc=nb=0
    for b in msg:
        acc|=b<<nb;nb+=8
        while nb>=7:packed.append(acc&127);acc>>=7;nb-=7
    if nb:packed.append(acc&127)
    wire=bytes([0xf0,*packed,0xf7])
    def array(name,raw):return 'static const uint8_t '+name+'[] = {\n'+','.join(str(b) for b in raw)+'\n};\n'
    (gen/'smk37_loader_image.h').write_text('/* Generated RAM-only SMK updater. */\n'+array('smk37_loader_image',data)+array('smk37_update_identity_wire',wire))
    report={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'entry':'0x1c0a800','hardware_validated':False,'power_loss_recovery':'hardware adapter required if application programming was interrupted'}
    (out/'smk37-loader-manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    print('SMK RAM loader:',report)
