// SPDX-License-Identifier: GPL-3.0-only
import fs from 'node:fs';import vm from 'node:vm';import assert from 'node:assert/strict';import crypto from 'node:crypto';
const html=fs.readFileSync('web/editor.html','utf8'),proto=html.split('/*PROTO-BEGIN*/')[1].split('/*PROTO-END*/')[0];const ctx={console,setTimeout,clearTimeout};vm.createContext(ctx);vm.runInContext(proto+';globalThis.api={req,parse,frame,unframe,CMD};',ctx);const a=ctx.api;
const wasm=fs.readFileSync('web/audio/smk-engine.wasm'),manifest=JSON.parse(fs.readFileSync('web/audio/smk-build.json'));assert.equal(crypto.createHash('sha256').update(wasm).digest('hex'),manifest.sha256);
let {instance:{exports:e}}=await WebAssembly.instantiate(wasm);e.synth_init();let mem=new Uint8Array(e.memory.buffer);
let metadata;
function request(r){const bytes=a.frame(...r).slice(1,-1);mem.set(bytes,e.fw_request());const n=e.fw_command(bytes.length),out=mem.slice(e.fw_reply(),e.fw_reply()+n);const frames=[];let pos=0;while(pos<out.length){let end=out.indexOf(247,pos);assert.ok(end>=0,'complete SysEx');const f=a.unframe(out.slice(pos,end+1));if(f&&f.cmd===r[0])frames.push(a.parse[f.cmd](f.a,metadata));pos=end+1;}assert.ok(frames.length,'reply for '+r[0]);return frames.at(-1);}
function render(blocks=100){let peak=0;for(let i=0;i<blocks;i++){const p=e.synth_render();for(const n of new Int32Array(e.memory.buffer,p,64))peak=Math.max(peak,Math.abs(n));}return peak;}
const info=request(a.req.info());metadata=info;assert.ok(info.engines.includes('FM6'));assert.ok(info.engines.includes('PROPHET'));assert.ok(info.engines.includes('CZ-1'));assert.ok(info.pcount>=104);console.log('Native protocol',info.version,info.pcount,info.gcount);
for(let i=0;i<info.pcount;i++)request(a.req.desc(0,i));for(let i=0;i<info.gcount;i++)request(a.req.desc(1,i));
for(const engine of [19,12,15,3,10]){request(a.req.set(1,20,engine));render(20);e.fw_event(2,12,110);const peak=render(200);e.fw_event(2,12,0);render(80);assert.ok(peak>0,'engine '+engine+' audio');console.log('Engine',engine,'peak',peak);}
request(a.req.set(1,20,0));render(30);for(let k=0;k<37;k++){e.fw_event(2,k,100);assert.ok(render(20)>0,'key '+k);e.fw_event(2,k,0);}e.fw_release();render(100);
for(let k=0;k<16;k++){e.fw_event(3,k,110);render(20);let s=new Uint32Array(e.memory.buffer,e.fw_status(),16);assert.ok(s[3]&(1<<k),'pad feedback '+k);e.fw_event(3,k,0);}e.fw_release();render(100);
request(a.req.set(0,1,0));request(a.req.set(0,2,0));request(a.req.set(0,3,127));request(a.req.set(0,4,0));request(a.req.set(0,37,0));render(40);e.fw_event(6,0,1);e.fw_event(2,12,110);render(100);e.fw_event(2,12,0);assert.ok(render(100)>0,'pedal sustains');e.fw_event(6,0,0);render(10000);assert.equal(render(100),0,'pedal up releases');
request(a.req.set(0,0,77));const ptr=e.fw_snapshot(),size=e.fw_snapshot_size(),saved=mem.slice(ptr,ptr+size);request(a.req.set(0,0,12));mem.set(saved,ptr);assert.equal(e.fw_restore(),0);assert.equal(request(a.req.dump()).p[0],77,'restore current song');mem[ptr]^=255;assert.notEqual(e.fw_restore(),0,'reject corrupt session');
let s=new Uint32Array(e.memory.buffer,e.fw_status(),16);assert.equal(s[1],0);assert.equal(s[2],0);assert.ok(mem.slice(e.fw_screen(),e.fw_screen()+240*240*2).some(x=>x),'native LCD populated');console.log('PASS: engines, editor descriptors, 37 keys, 16 pads, sustain, releases, session restore and corruption rejection');

// Fill all project slots with distinct sounds; all 32 patterns share the native format.
for(let track=0;track<4;track++){
  request(a.req.track(track));
  for(let bank=0;bank<8;bank++){
    assert.equal(request(a.req.pattern(track,1,bank)).rc,0);
    request(a.req.stepSet(0,{n:1,notes:[36+track*8+bank,0,0,0],time:0,flags:0,vel:80+bank}));
  }
}
request(a.req.track(0));
for(let slot=0;slot<4;slot++){request(a.req.set(0,0,60+slot));assert.equal(request(a.req.project(1,slot)).used,1);}
request(a.req.upStore(0,'BROWSER TEST'));assert.equal(request(a.req.upGet(0)).name,'BROWSER TEST');
const nativeSaved=new Map();
for(const engine of [12,15,19]){
  request(a.req.preset(engine,1));render(10);
  assert.equal(request(a.req.native(engine,3,0,0)).rc,0,'store native '+engine);
  nativeSaved.set(engine,Array.from(request(a.req.native(engine,1,0)).raw));
}
assert.ok([0,3].includes(request(a.req.uiSet(0,2)).rc));
assert.ok([0,3].includes(request(a.req.uiSet(4,2)).rc));
assert.ok([0,3].includes(request(a.req.favSet(0,1,true)).rc));
assert.ok([0,3].includes(request(a.req.favSet(metadata.namespace[19],0,true)).rc));
const rows=[{banks:[1,2,3,4],repeat:3}];assert.equal(request(a.req.bankSong(1,rows)).rc,0);
request(a.req.set(0,0,77));
const fullPtr=e.fw_snapshot(),fullSize=e.fw_snapshot_size();assert.ok(fullPtr);const full=mem.slice(fullPtr,fullPtr+fullSize);
({instance:{exports:e}}=await WebAssembly.instantiate(wasm));e.synth_init();mem=new Uint8Array(e.memory.buffer);
assert.equal(request(a.req.project(2,0)).used,0,'fresh runtime has no project');
const restorePtr=e.fw_snapshot();mem.set(full,restorePtr);assert.equal(e.fw_restore(),0,'restore into fresh runtime');
assert.equal(request(a.req.dump()).p[0],77);
assert.equal(request(a.req.uiState()).palette,2);assert.equal(request(a.req.uiState()).click,2);
assert.equal(request(a.req.favGet(0,1,1)).values[0],true);
assert.equal(request(a.req.favGet(metadata.namespace[19],0,1)).values[0],true);
assert.equal(request(a.req.upGet(0)).name,'BROWSER TEST');
for(const [engine,raw] of nativeSaved)assert.deepEqual(Array.from(request(a.req.native(engine,1,0)).raw),raw,'native bank '+engine);
assert.equal(JSON.stringify(request(a.req.bankSong()).rows),JSON.stringify(rows));
for(let slot=0;slot<4;slot++){
  assert.equal(request(a.req.project(2,slot)).used,1);
  request(a.req.project(0,slot));assert.equal(request(a.req.dump()).p[0],60+slot,'saved slot '+slot);
}
for(let track=0;track<4;track++){
  request(a.req.track(track));
  for(let bank=0;bank<8;bank++){
    request(a.req.pattern(track,1,bank));
    assert.equal(request(a.req.stepGet(0)).notes[0],36+track*8+bank,'pattern '+track+'/'+bank);
  }
}
// A forged checksum cannot bypass the native bank validation. Failed imports keep music and held keys.
request(a.req.set(0,0,91));e.fw_event(2,12,100);const beforeKey=new Uint32Array(e.memory.buffer,e.fw_status(),16)[1];
function rehash(b){let h=0x811c9dc5;for(const x of b.subarray(0,-4))h=Math.imul(h^x,16777619)>>>0;new DataView(b.buffer,b.byteOffset,b.byteLength).setUint32(b.length-4,h,true);}
for(const offset of [0,4,8,32,100,full.length-1]){
  const damaged=full.slice();damaged[offset]^=255;if(offset!==full.length-1)rehash(damaged);
  mem.set(damaged,restorePtr);assert.notEqual(e.fw_restore(),0,'reject damaged section '+offset);
  assert.equal(request(a.req.dump()).p[0],91,'preserve music on rejection');
  assert.equal(new Uint32Array(e.memory.buffer,e.fw_status(),16)[1],beforeKey,'preserve held note on rejection');
}
e.fw_release();
const {changesSession}=await import('data:text/javascript;base64,'+Buffer.from(fs.readFileSync('web/audio/session-commands.js')).toString('base64'));
for(const req of [a.req.info(),a.req.dump(),a.req.uiState(),a.req.track(),a.req.trackMix(0),a.req.pattern(0),a.req.native(12,1,0),a.req.prophetGet(0)])assert.equal(changesSession(a.frame(...req)),false,'poll never postpones autosave');
for(const req of [a.req.native(12,3,0,0),a.req.prophetEdit(0,1,2),a.req.pattern(0,1,1),a.req.upStore(0,'X'),a.req.uiSet(4,2)])assert.equal(changesSession(a.frame(...req)),true,'native edit schedules save');
console.log('PASS: full session',fullSize,'bytes; fresh-runtime projects A-D, 32 patterns, FM6/CZ/Prophet banks, general preset, favorites, settings, song chain and atomic rejection');
