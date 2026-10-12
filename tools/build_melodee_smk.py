#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build a local development candidate with the bounded SMK RAM updater."""
import hashlib,json,re,sys,subprocess
from pathlib import Path
import build
from build_smk37_loader import build_loader
from smk_storage_audit import audit_storage
out=build.SRC/'build/melodee-smk-1.1.0-b1';out.mkdir(parents=True,exist_ok=True)
gen=build.SRC/'build/gen';gen.mkdir(parents=True,exist_ok=True)
(gen/'smk37_unit.h').write_text('''/* Public profile: calibration is validated on each unit, never replaced. */
#define SMK37_PUBLIC_BUILD 1
#define SMK37_UNIT_TRIM_CRC 0u
#define SMK37_UNIT_HEAD_CRC 0x3a04ad23u
#define SMK37_UNIT_SAMPLE_CRCS {0,0,0}
#define SMK37_UPDATE_PRODUCT "SMK-37_941"
#define SMK37_UPDATE_VERSION "MELODEE SMK 1.1.0-b1"
#define SMK37_RELEASE_VERSION "1.1.0-b1"
''')
build_loader(out,gen,'SMK-37_941')
flags=[*build.CFLAGS,'-DMELODEE_DUAL_CORE=0','-DMELODEE_CACHE_RAM=0','-DSMK37_ENABLE_USB_AUDIO=1','-Ifirmware/hal','-Ifirmware/src','-Ibuild/gen']
build.tc_all(('cc',*flags,'-S','-emit-llvm','-Xclang','-disable-llvm-optzns','-c',build.FW/'src/melodee_smk_app.c','-o',out/'app.ll'),('cc',*flags,'-c',build.FW/'smk37_crt0.S','-o',out/'crt0.o'),('cc',*flags,'-c',build.FW/'hal/fm1_vec.S','-o',out/'vec.o'))
subprocess.run([sys.executable,str(build.SRC/'tools/size_fns.py'),str(out/'app.ll'),str(out/'app-size.ll')],check=True)
build.tc('cc',*flags,'-c',out/'app-size.ll','-o',out/'app.o')
elf=out/'melodee-smk.elf'
build.tc('pi32v2/bin/ld','--gc-sections','-T',build.FW/'app.ld',out/'crt0.o',out/'vec.o',out/'app.o','-o',elf)
symbols=build.tc('common/bin/objdump','-t',elf);(out/'symbols.txt').write_text(symbols)
(out/'sections.txt').write_text(build.tc('common/bin/objdump','-h',elf))
def sym(name):return int(re.search(r'^([0-9a-f]+) .*\s'+name+r'$',symbols,re.M)[1],16)
image=bytearray()
for section,address in [('.text',build.APP_XIP),('.ram_text',sym('_rt_load')),('.dsp_text',sym('_dsp_load')),('.dsp_tables',sym('_dt_load')),('.data',sym('_data_load'))]:
 blob=out/(section[1:]+'.bin');build.tc('common/bin/objcopy','-O','binary','-j',section,elf,blob)
 raw=blob.read_bytes() if blob.exists() else b''
 if not raw:continue
 off=address-build.APP_XIP
 if off<len(image):raise SystemExit('overlapping image sections')
 image.extend(b'\xff'*(off-len(image)));image.extend(raw)
image.extend(b'\xff'*(-len(image)%4))
if len(image)>0x8dfbc or sym('_start')!=build.APP_XIP:raise SystemExit('SMK application bounds invalid')
for forbidden in ['fm1_input_init','fm1_adc_init','fm1_audio_init','ota_session','fm1_multicore_start']:
 if re.search(r'\b'+forbidden+r'$',symbols,re.M):raise SystemExit('wrong-board or disabled routine linked: '+forbidden)
disassembly=build.tc('common/bin/objdump','-d','-j','.ram_text',elf)
(out/'ram-disassembly.txt').write_text(disassembly)
audit=audit_storage(build.SRC,out,symbols,disassembly)
audit_text=json.dumps(audit,indent=2)+'\n'
(out/'storage-audit.json').write_text(audit_text)
name='melodee-smk-1.1.0-b1-candidate.bin';(out/name).write_bytes(image)
manifest={'firmware':'Melodee SMK 1.1.0-b1','board':'SMK-37 Pro & Elite','installable':True,'release_channel':'local-development','update_product':'SMK-37_941','update_version':'MELODEE SMK 1.1.0-b1','hardware_tested':False,'upstream':'keremimo/melodee','upstream_revision':'01a78bd','donor_revision':'090588d','image':name,'bytes':len(image),'sha256':hashlib.sha256(image).hexdigest(),'entry':hex(sym('_start')),'bss_end':hex(sym('_bss_end')),'pool_end':hex(sym('_pool_end')),'resource_bytes':sym('_resource_end')-sym('_resource_start'),'dual_core':False,'cache_ram':False,'persistent_storage':True,'storage_initialization':'backup-required','storage_audit_sha256':hashlib.sha256(audit_text.encode()).hexdigest(),'usb_audio':True,'usb_audio_rate':44100,'trs_midi_out':True,'usb_midi':True,'updater':True}
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps(manifest,indent=2))
