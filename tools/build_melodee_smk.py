#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build a local development candidate with the bounded SMK RAM updater."""
import argparse,hashlib,json,re,sys,subprocess
from pathlib import Path
import build
from build_smk37_loader import build_loader
from smk_storage_audit import audit_storage
from smk_multicore_audit import audit_multicore
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--dual-core',action='store_true',help='Build the CPU1 variant; publication requires an explicitly pinned release')
parser.add_argument('--generate',action='store_true',help='Regenerate UI and synth headers first')
args=parser.parse_args()
if args.generate:
 build.generate()
 subprocess.run([sys.executable,str(build.SRC/'tools/gen_909_tables.py'),str(build.GEN/'x0x_drum_tables.h')],check=True)
version='1.1.0-b1-dc7' if args.dual_core else '1.1.0-b1'
out=build.SRC/('build/melodee-smk-'+version);out.mkdir(parents=True,exist_ok=True)
# Variant-specific unit/loader headers cannot leak into a baseline build.
gen=out/'gen';gen.mkdir(parents=True,exist_ok=True)
(gen/'smk37_unit.h').write_text('''/* Public profile: calibration is validated on each unit, never replaced. */
#define SMK37_PUBLIC_BUILD 1
#define SMK37_UNIT_TRIM_CRC 0u
#define SMK37_UNIT_HEAD_CRC 0x3a04ad23u
#define SMK37_UNIT_SAMPLE_CRCS {0,0,0}
#define SMK37_UPDATE_PRODUCT "SMK-37_941"
#define SMK37_UPDATE_VERSION "MELODEE SMK @VERSION@"
#define SMK37_RELEASE_VERSION "@VERSION@"
'''.replace('@VERSION@',version))
build_loader(out,gen,'SMK-37_941')
flags=[*build.CFLAGS,f'-DMELODEE_DUAL_CORE={int(args.dual_core)}','-DMELODEE_CACHE_RAM=0','-DSMK37_ENABLE_USB_AUDIO=1','-Ifirmware/hal','-Ifirmware/src',f'-I{gen.relative_to(build.SRC)}','-Ibuild/gen']
build.tc_all(('cc',*flags,'-S','-emit-llvm','-Xclang','-disable-llvm-optzns','-c',build.FW/'src/melodee_smk_app.c','-o',out/'app.ll'),('cc',*flags,'-c',build.FW/'smk37_crt0.S','-o',out/'crt0.o'),('cc',*flags,'-c',build.FW/'hal/fm1_vec.S','-o',out/'vec.o'))
subprocess.run([sys.executable,str(build.SRC/'tools/size_fns.py'),str(out/'app.ll'),str(out/'app-size.ll'),'--smk'],check=True)
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
for forbidden in ['fm1_input_init','fm1_adc_init','fm1_audio_init','ota_session']:
 if re.search(r'\b'+forbidden+r'$',symbols,re.M):raise SystemExit('wrong-board or disabled routine linked: '+forbidden)
dsp_disassembly=build.tc('common/bin/objdump','-d','-j','.dsp_text',elf)
(out/'dsp-disassembly.txt').write_text(dsp_disassembly)
multicore_audit=audit_multicore(symbols,dsp_disassembly,args.dual_core)
(out/'multicore-audit.json').write_text(json.dumps(multicore_audit,indent=2)+'\n')
disassembly=build.tc('common/bin/objdump','-d','-j','.ram_text',elf)
(out/'ram-disassembly.txt').write_text(disassembly)
audit=audit_storage(build.SRC,out,symbols,disassembly)
audit_text=json.dumps(audit,indent=2)+'\n'
(out/'storage-audit.json').write_text(audit_text)
name='melodee-smk-'+version+'-candidate.bin';(out/name).write_bytes(image)
manifest={'firmware':'Melodee SMK '+version,'board':'SMK-37 Pro & Elite','installable':True,'release_channel':'local-development','update_product':'SMK-37_941','update_version':'MELODEE SMK '+version,'hardware_tested':False,'upstream':'keremimo/melodee','upstream_revision':'01a78bd','donor_revision':'090588d','image':name,'bytes':len(image),'sha256':hashlib.sha256(image).hexdigest(),'entry':hex(sym('_start')),'bss_end':hex(sym('_bss_end')),'pool_end':hex(sym('_pool_end')),'resource_bytes':sym('_resource_end')-sym('_resource_start'),'dual_core':args.dual_core,'multicore_audit':multicore_audit,'cache_ram':False,'persistent_storage':True,'storage_initialization':'backup-required','storage_audit_sha256':hashlib.sha256(audit_text.encode()).hexdigest(),'usb_audio':True,'usb_audio_rate':44100,'trs_midi_out':True,'usb_midi':True,'updater':True}
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps(manifest,indent=2))
