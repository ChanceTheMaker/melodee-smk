# SPDX-License-Identifier: GPL-3.0-only
"""Offline checks for private backup decoding and the destructive-setup plan gate."""
import hashlib
import json
import sys
import tempfile
import unittest
import zlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from smk_storage_backup import SIZE,read32,u32,unpack,request
from smk_storage_initialize import plan

class StorageToolsTest(unittest.TestCase):
    def test_codec_and_write_rejection(self):
        for n in (0,127,128,393216,0xffffffff): self.assertEqual(read32(u32(n)),n)
        raw=bytes(range(256));encoded=[]
        for i in range(0,len(raw),7):
            group=raw[i:i+7]
            encoded += [sum(((b>>7)&1)<<j for j,b in enumerate(group))]+[b&127 for b in group]
        self.assertEqual(unpack(encoded,256),raw)
        with self.assertRaises(ValueError): request(None,None,2)
        with self.assertRaises(ValueError): unpack([127,0],1)
        with self.assertRaises(ValueError): read32([0,0,0,0,16])

    def test_setup_requires_unchanged_uninterrupted_backup(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);blob=b'\xff'*SIZE
            checksum={'sha256':hashlib.sha256(blob).hexdigest(),'crc32':zlib.crc32(blob)}
            meta={'matching':True,'device_initialization_gate_ready':True,'read_retries':0,
                  'bytes':SIZE,'unit':42,'passes':[checksum,checksum]}
            for i in (1,2): (p/f'pass-{i}.bin').write_bytes(blob)
            (p/'manifest.json').write_text(json.dumps(meta))
            self.assertEqual(plan(p)['crc32'],checksum['crc32'])
            (p/'pass-2.bin').write_bytes(b'\x00'+blob[1:])
            with self.assertRaises(ValueError): plan(p)
            (p/'pass-2.bin').write_bytes(blob)
            for changes in ({'read_retries':1},{'matching':False},{'device_initialization_gate_ready':False}):
                (p/'manifest.json').write_text(json.dumps(dict(meta,**changes)))
                with self.assertRaises(ValueError): plan(p)

if __name__=='__main__': unittest.main()
