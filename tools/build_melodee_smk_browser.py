#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build the native Melodee SMK browser runtime. No hardware access."""
import argparse, hashlib, json, os, pathlib, shutil, subprocess, sys
ROOT=pathlib.Path(__file__).resolve().parent.parent
EXPORTS='synth_init synth_midi synth_render fw_event fw_release fw_screen fw_status fw_request fw_reply fw_command fw_changed fw_snapshot fw_snapshot_size fw_restore'.split()
def main():
 p=argparse.ArgumentParser();p.add_argument('--zig',default=os.environ.get('ZIG') or shutil.which('zig'));p.add_argument('--generate',action='store_true');a=p.parse_args();os.chdir(ROOT)
 if not a.zig:p.error('Pass --zig /path/to/zig (tested with Zig 0.13.0)')
 if a.generate:
  import build
  build.generate()
  subprocess.run([sys.executable,'tools/gen_909_tables.py','build/gen/x0x_drum_tables.h'],check=True)
 required=['ui_fonts.h','ui_icons.h','ui_keycaps.h','ui_palettes.h','melodee_tables.h','melodee_fm6.h','melodee_cz1.h','melodee_prophet_factory.h','x0x_drum_tables.h']
 for f in required:
  if not (ROOT/'build/gen'/f).is_file():p.error('Missing build/gen/'+f+'; run with --generate (Pillow with Raqm and fonttools required)')
 out=ROOT/'web/audio/smk-engine.wasm'
 env=os.environ.copy();env.setdefault('ZIG_GLOBAL_CACHE_DIR',str(ROOT/'build/zig-cache'))
 cmd=[a.zig,'cc','-target','wasm32-freestanding','-O2','-fno-builtin','-fwrapv','-nostdlib','-Wl,--no-entry','-Wl,-z,stack-size=0x80000','-Wl,--initial-memory=0x1000000','-Wl,--max-memory=0x1000000',*[f'-Wl,--export={x}' for x in EXPORTS],'-Iweb/audio/include','-Ibuild/gen','-Ifirmware/hal','web/audio/melodee-smk-engine.c','-o',str(out)]
 subprocess.run(cmd,check=True,env=env)
 raw=out.read_bytes();meta={'version':'1.1.0-dev1','engine':'Melodee SMK','sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}
 out.with_name('smk-build.json').write_text(json.dumps(meta,indent=2)+'\n');print(json.dumps(meta))
if __name__=='__main__':main()
