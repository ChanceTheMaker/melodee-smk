# SPDX-License-Identifier: GPL-3.0-only
import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from smk_audio_input import select_input
HOSTS=[{'name':'MME'},{'name':'Windows WASAPI'},{'name':'Windows WDM-KS'}]
def dev(name='Line (Melodee In)',host=1,channels=4):
 return {'name':name,'hostapi':host,'max_input_channels':channels}
class SelectTest(unittest.TestCase):
 def test_prefer_wasapi(self):
  self.assertEqual(select_input([dev(host=0),dev(),dev(host=2)],HOSTS)[0],1)
 def test_old_name(self):
  self.assertEqual(select_input([dev(name='Synthesizer (Melodee In)')],HOSTS)[0],0)
 def test_no_microphone_fallback(self):
  for devices in ([dev(name='Microphone')],[dev(host=2)],[dev(channels=2)],[]):
   with self.assertRaises(RuntimeError):select_input(devices,HOSTS)
 def test_ambiguous(self):
  with self.assertRaises(RuntimeError):select_input([dev(),dev()],HOSTS)
if __name__=='__main__':unittest.main()
