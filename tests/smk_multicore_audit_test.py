# SPDX-License-Identifier: GPL-3.0-only
import sys, unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from smk_multicore_audit import audit_multicore

class MulticoreAuditTest(unittest.TestCase):
    def setUp(self):
        self.symbols = '\n'.join([
            '02000130 g F .text 00000030 fm1_core1_start',
            '01c01000 g F .dsp_text 00000020 fm1_core1_main',
            '01c01020 g F .dsp_text 00000100 audio_worker_loop',
            '01c01000 g .dsp_text 00000000 _dsp_start',
            '01c01120 g .dsp_text 00000000 _dsp_end',
            '01c40000 g .pool 00000000 _pool_end',
            '01c40000 g .cpu1_stacks 00000000 _cpu1_stacks_lo',
            '01c42000 g .cpu1_stacks 00000000 _cpu1_stacks_hi',
            '01c42000 g .cpu1_stacks 00000000 _resource_start',
            '01c74000 g .cpu1_stacks 00000000 _resource_end',
        ])
        self.asm = '\n'.join(f'{0x1c01020+i*2:08x}: 20 00 csync' for i in range(30))
        self.asm += '\n01c01060: 20 00 call r3'
    def test_valid(self):
        self.assertTrue(audit_multicore(self.symbols,self.asm,True)['idle_in_sram'])
    def test_wrong_stack(self):
        with self.assertRaises(ValueError): audit_multicore(self.symbols.replace('01c42000','01c41000'),self.asm,True)
    def test_overlap(self):
        with self.assertRaises(ValueError): audit_multicore(self.symbols.replace('01c40000 g .pool','01c41000 g .pool'),self.asm,True)
    def test_xip_worker(self):
        with self.assertRaises(ValueError): audit_multicore(self.symbols.replace('01c01020 g F .dsp_text','02001020 g F .text'),self.asm,True)
    def test_direct_call(self):
        with self.assertRaises(ValueError): audit_multicore(self.symbols,self.asm.replace('call r3','call 2 <helper : 02000120 >'),True)
    def test_external_branch(self):
        with self.assertRaises(ValueError): audit_multicore(self.symbols,self.asm+'\n01c01062: 20 00 goto 2 <helper : 02000120 >',True)
    def test_internal_branch(self):
        audit_multicore(self.symbols,self.asm+'\n01c01062: 20 00 goto -2 <poll : 01c01020 >',True)
    def test_empty(self):
        with self.assertRaises(ValueError): audit_multicore(self.symbols,'',True)
    def test_no_callback(self):
        with self.assertRaises(ValueError): audit_multicore(self.symbols,self.asm.replace('call r3','csync'),True)
    def test_serial_rejects_worker(self):
        with self.assertRaises(ValueError): audit_multicore(self.symbols,self.asm,False)
    def test_serial(self):
        symbols='01c40000 g .cpu1_stacks 00000000 _cpu1_stacks_lo\n01c40000 g .cpu1_stacks 00000000 _cpu1_stacks_hi'
        self.assertFalse(audit_multicore(symbols,'',False)['enabled'])
if __name__ == '__main__': unittest.main()
