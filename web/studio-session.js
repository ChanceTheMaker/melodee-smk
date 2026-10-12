// SPDX-License-Identifier: GPL-3.0-only
import {mountMIDIInput} from './midi-input.js';
const $=id=>document.getElementById(id),studio=window.MelodeeStudio;
let switching=false;
function refresh(){const browser=studio.browser,dev=studio.device;for(const [id,on] of [['mode-browser',browser],['mode-device',!browser]]){ $(id).setAttribute('aria-pressed',String(on));$(id).disabled=switching; }$('browser-session').hidden=$('studio-scope').hidden=!browser;window.MelodeePlay?.update(dev?.dump?{output:dev.output,track:dev.sel||0,browser,drumChannel:4,lanes:Array.from({length:16},(_,i)=>[[36,38,42,46,41,43,45,49,51,37,39,56,54,75,76,77][i]])}:null);if(browser&&studio.synth?.storageMessage)$('session-status').textContent=studio.synth.storageMessage;}
async function mode(browser){if(switching)return;switching=true;refresh();try{await (browser?studio.useBrowser():studio.useDevice());if(browser&&studio.synth?.node){studio.synth.volume(+$('audio-volume').value/100);refresh();studio.synth.showScope($('scope-canvas'));}}catch(e){$('status').textContent=e.message;}finally{switching=false;refresh();}}
$('mode-browser').onclick=()=>mode(true);$('mode-device').onclick=()=>mode(false);
$('audio-volume').oninput=()=>studio.synth?.volume(+$('audio-volume').value/100);
$('session-panic').onclick=()=>{studio.release();studio.synth?.panic();};
$('session-play').onclick=()=>{studio.synth?.panel(0,16,1);setTimeout(()=>studio.synth?.panel(0,16,0),80);};
$('session-export').onclick=async()=>{try{const data=await studio.synth.exportSession(),url=URL.createObjectURL(new Blob([JSON.stringify(data)],{type:'application/json'})),a=document.createElement('a');a.href=url;a.download='melodee-smk-browser-session.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}catch(e){$('status').textContent=e.message;}};
$('session-import').onclick=()=>$('session-file').click();
$('session-file').onchange=async()=>{const f=$('session-file').files[0];$('session-file').value='';if(!f)return;try{if(f.size>2000000)throw Error('Session is too large');studio.release();await studio.synth.importSession(JSON.parse(await f.text()));await mode(true);}catch(e){$('status').textContent=e.message;}};
mountMIDIInput({container:$('browser-session'),getSynth:()=>studio.synth,active:()=>studio.browser&&!!studio.device?.dump,enableAudio:async()=>{if(!studio.browser)await mode(true);else await studio.synth?.context?.resume();}});
document.addEventListener('click',async e=>{const a=e.target.closest('a[href]');if(!a||!studio.browser||!studio.synth?.node||e.defaultPrevented||e.button||e.ctrlKey||e.metaKey||e.shiftKey||a.target||a.download)return;const url=new URL(a.href);if(url.origin!==location.origin||url.pathname===location.pathname)return;e.preventDefault();try{studio.release();await studio.synth.save();await studio.synth.stop();location.assign(url.href);}catch(err){$('status').textContent=err.message;}});
setInterval(refresh,500);refresh();if(new URLSearchParams(location.search).get('audio')==='1')mode(true);

document.addEventListener('pointerdown',e=>{if(studio.browser&&e.target.closest('#play-keys,#mode-browser'))studio.synth?.context?.resume().catch(()=>{});},true);document.addEventListener('keydown',()=>{if(studio.browser)studio.synth?.context?.resume().catch(()=>{});},true);
