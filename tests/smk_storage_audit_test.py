# SPDX-License-Identifier: GPL-3.0-only
import sys, unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from smk_storage_audit import audit_ram
class AuditTest(unittest.TestCase):
 def setUp(self):
  self.symbols='01c00000 .ram_text 00000000 _rt_start\n01c00b68 .ram_text 00000000 _rt_end\n'
  for i,n in enumerate(['fl_erase4k_ram','fl_prog_ram','fl_read_ram','fl_jedec_ram']):
   self.symbols+=f'{0x1c00000+i*128:08x} l F .ram_text 00000080 {n}\n'
  self.asm='\n'.join(f' {0x1c00000+i*2:x}: 20 00 \tcsync' for i in range(100))
 def test_valid(self): self.assertEqual(audit_ram(self.symbols,self.asm)['instructions_checked'],100)
 def test_missing(self):
  with self.assertRaises(ValueError):audit_ram(self.symbols.replace('fl_prog_ram','wrong'),self.asm)
 def test_xip(self):
  with self.assertRaises(ValueError):audit_ram(self.symbols.replace('01c00080 l F .ram_text','02000000 l F .text'),self.asm)
 def test_call(self):
  with self.assertRaises(ValueError):audit_ram(self.symbols,self.asm.replace('csync','call 2 <helper : 2000000 >',1))
 def test_external_branch(self):
  with self.assertRaises(ValueError):audit_ram(self.symbols,self.asm.replace('csync','goto 2 <helper : 2000000 >',1))
 def test_indirect_branch(self):
  with self.assertRaises(ValueError):audit_ram(self.symbols,self.asm.replace('csync','goto r3',1))
 def test_empty(self):
  with self.assertRaises(ValueError):audit_ram(self.symbols,'')
if __name__=='__main__':unittest.main()
