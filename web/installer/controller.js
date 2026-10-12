// SPDX-License-Identifier: GPL-3.0-only
import {buildSmkPublicPackage} from './smk37-package.js';
import {parsePackage,installSmk,installStockSmk} from './smk37-update.js?v=smk-flow-r1';
import {STOCK_SOURCES} from './smk37-stock.js';
const $=id=>document.getElementById(id);
const dialog=document.getElementById('loader-confirm');
async function confirmGeneric({identity,port,model}){
  document.getElementById('loader-identity').textContent=identity+' on '+port;
  document.getElementById('loader-model').textContent=model;
  log('Confirm the connected Elite in the dialog to continue this installation.');
  dialog.returnValue='cancel';
  return new Promise(resolve=>{
    dialog.addEventListener('close',()=>resolve(dialog.returnValue==='continue'),{once:true});
    dialog.showModal();
  });
}
let manifest=null,prepared=null,preparing=false,installing=false,generation=0;
const support=window.isSecureContext&&typeof navigator.requestMIDIAccess==='function';
const log=text=>{const line=new Date().toISOString()+' '+text;console.log(line);$('log').textContent+= '\n'+line;$('log').scrollTop=$('log').scrollHeight;};
function status(text,error=false){$('status').textContent=text;$('status').dataset.error=String(error);}
function controls(){
  $('install').disabled=!support||!prepared||!$('confirm').checked||preparing||installing;
  for(const id of ['model','route','stock-file','prepare'])$(id).disabled=installing;
  $('prepare').disabled=installing||preparing;
  $('confirm').disabled=installing;
}
async function fetchChecked(url){const r=await fetch(url,{cache:'no-store',signal:AbortSignal.timeout(30000)});if(!r.ok)throw Error('Unable to load '+url+' (HTTP '+r.status+').');return r;}
async function prepare(){
  const request=++generation,model=$('model').value,file=$('stock-file').files[0];prepared=null;preparing=true;$('confirm').checked=false;controls();status('Preparing Melodee for '+model+'… No device access.');
  try{
    const m=await (await fetchChecked('firmware/candidate.json')).json();
    if(!['local-development','public-development'].includes(m.release_channel)||!m.installable||!m.updater||m.hardware_tested!==false||m.update_product!=='SMK-37_940'||m.update_version!=='MELODEE SMK 1.1.0-dev1'||!/^[a-z0-9.-]+-candidate\.bin$/.test(m.image))throw Error('The staged manifest is not this reviewed Melodee development candidate.');
    const app=new Uint8Array(await (await fetchChecked('firmware/'+m.image)).arrayBuffer());
    const stock=new Uint8Array(await (file?file.arrayBuffer():fetchChecked(STOCK_SOURCES[model]).then(r=>r.arrayBuffer())));
    const pkg=await buildSmkPublicPackage(stock,app,{localCandidate:true,product:m.update_product,version:m.update_version,bytes:m.bytes,sha256:m.sha256});
    if(pkg.stockModel!==model)throw Error('That file is for '+pkg.stockModel+'. Choose the matching model or its official 016 file.');
    await parsePackage(pkg.bytes,pkg.sha);
    if(request!==generation)return;
    manifest=m;prepared=pkg;$('version').textContent=m.firmware;$('hash').textContent='Application SHA-256: '+m.sha256;
    status(support?m.firmware+' ready for '+model+'. Confirm the development notice to enable Install.':'Files verified. Installing requires desktop Chrome or Edge on HTTPS or localhost with Web MIDI support.',!support);
    log('Prepared '+pkg.version+' for '+model+'; package SHA-256 '+pkg.sha+'. No MIDI ports opened.');
  }catch(e){if(request!==generation)return;status(e.message+' If the automatic official-file download failed, use the local-file fallback above.',true);log('Preparation stopped: '+e.message);}
  finally{if(request===generation){preparing=false;controls();}}
}
$('model').addEventListener('change',prepare);$('stock-file').addEventListener('change',prepare);$('prepare').addEventListener('click',prepare);$('confirm').addEventListener('change',controls);
$('route').addEventListener('change',()=>{$('confirm').checked=false;$('route-note').textContent=$('route').value==='stock'?'Automatically detects stock firmware, an active loader, or Felucca / Sloop / Melodee SMK.':'Requires the portable SMK updater (identity 922 or newer). Older private builds are refused before starting an update.';controls();});
$('install').addEventListener('click',async()=>{
  if($('install').disabled||!prepared)return;
  const pkg=prepared,route=$('route').value;installing=true;controls();
  try{
    await parsePackage(pkg.bytes,pkg.sha);
    status('Requesting MIDI permission. Keep the SMK powered and USB connected.');
    const access=await navigator.requestMIDIAccess({sysex:true});
    log('Starting explicit '+route+' installation.');
    await (route==='stock'?installStockSmk:installSmk)(access,pkg,text=>{status(text);log(text);},{confirmGeneric});
    status('Installed '+manifest.firmware+'. Check the screen, keys and sound on the SMK. Device saves remain disabled.');
    $('confirm').checked=false;
  }catch(e){status(e.message,true);log('Installation stopped: '+e.message);$('confirm').checked=false;}
  finally{installing=false;controls();}
});
window.addEventListener('beforeunload',event=>{if(installing){event.preventDefault();event.returnValue='';}});
$('copy-log').addEventListener('click',async()=>{try{await navigator.clipboard.writeText($('log').textContent);$('copy-log').textContent='Copied';}catch{status('Select and copy the installation log below.',true);}});
prepare();
