// SPDX-License-Identifier: GPL-3.0-only
// Offline only: model/version gates, handoff, resume and verified reboot.
import assert from 'node:assert/strict';
import {Updater} from './installer/fm1ota.js?v=progress1';
import {installSmk,installStockSmk} from './installer/smk37-update.js?v=progress1';
const methods=['findAny','waitForAny','serve'];
const original=Object.fromEntries(methods.map(k=>[k,Updater.prototype[k]]));
const id=(model,version)=>({model,version,text:model+'_'+String(version).padStart(3,'0')});
let first,waiting,sends,transfers,finished;
const device=identity=>({id:identity,name:'Mock port',link:{send(){sends++;return true;},async close(){}}});
const reset=(initial,sequence=[])=>{first=initial;waiting=[...sequence];sends=0;transfers=[];finished=true;};
const pkg={format:2,stockModel:'SMK-37 Pro',product:'SMK-37_941',version:'MELODEE SMK 1.1.0-b1-dc1',bytes:new Uint8Array(64),image:new Uint8Array(128)};
try{
    Updater.prototype.findAny=async function(filter){return first&&filter(first)?device(first):null;};
    Updater.prototype.waitForAny=async function(filter){const next=waiting.shift();return next&&filter(next)?device(next):null;};
    Updater.prototype.serve=async function(link,bytes,address){transfers.push({bytes,address});return {finished};};
    for(const [model,version] of [['Doom-SMK-37',7],['SMK-37',941]]){
        reset(id(model,version),[id('ota-'+model,version),id('SMK-37',941)]);
        await installStockSmk({},pkg);
        assert.equal(transfers.length,1);assert.equal(transfers[0].bytes,pkg.bytes);
        assert.equal(transfers[0].address,0xf0000000);assert.equal(sends,2);
        reset(id('ota-'+model,version),[id('SMK-37',941)]);
        await installStockSmk({},pkg);assert.equal(transfers.length,1);assert.equal(sends,1);
    }
    for(const model of ['FM-1','Doom-FM-1','ota-Doom-FM-1','other']){
        reset(id(model,7));await assert.rejects(installStockSmk({},pkg),/No matching/);
        assert.equal(sends,0);assert.equal(transfers.length,0);
        await assert.rejects(installSmk({},pkg,()=>{},{device:device(id(model,7))}),/Not an SMK/);
        assert.equal(sends,0);
    }
    for(const identity of [id('Doom-SMK-37',6),id('ota-Doom-SMK-37',6),id('SMK-37',921)]){
        reset(identity);await assert.rejects(installStockSmk({},pkg),/older updater/);assert.equal(sends,0);
    }
    for(const next of [null,id('ota-Doom-FM-1',7),id('ota-SMK-37',941)]){
        reset(id('Doom-SMK-37',7),[next]);await assert.rejects(installStockSmk({},pkg),/did not appear/);
        assert.equal(transfers.length,0);
    }
    reset(id('Doom-SMK-37',7),[id('ota-Doom-SMK-37',6)]);
    await assert.rejects(installStockSmk({},pkg),/loader version/);assert.equal(transfers.length,0);
    reset(id('ota-Doom-SMK-37',7));finished=false;
    await assert.rejects(installStockSmk({},pkg),/Update stopped/);assert.equal(transfers.length,1);
    reset(id('ota-Doom-SMK-37',7));
    await assert.rejects(installStockSmk({},pkg),/identity did not return/);
    reset(id('ota-Doom-SMK-37',7),[id('SMK-37',940)]);
    await assert.rejects(installStockSmk({},pkg),/Unexpected returned version/);
    reset(id('SMK-37 Pro',16),[id('ota-SMK-37 Pro',16),id('SMK-37',941)]);
    await installStockSmk({},pkg);assert.deepEqual(transfers.map(x=>x.address),[0xe0000000,0xf0000000]);
    console.log('PASS: stock/portable/Doom SMK handoff and resume, cross-model and old-loader rejection, transfer failure, and exact Melodee restart identity.');
}finally{Object.assign(Updater.prototype,original);}
