// SPDX-License-Identifier: GPL-3.0-only
// Reuse Melodee's verified public package builders; never accept caller firmware.
import fs from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {buildSmkPublicPackage,smkLogicalImage} from '../web/installer/smk37-package.js';
import {buildSmkStock} from '../web/restore-stock/package-smk.js';
import {sha256hex} from '../web/fm1pkg.js';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const [action,mode,out]=process.argv.slice(2);
if(!['install','restore'].includes(action)||mode!=='public'||!out)throw Error('Invalid local installer request');
const stock=new Uint8Array(await fs.readFile(path.join(root,'build/smk37/stock-pro.fwsc')));
const original=await buildSmkStock(stock,'SMK-37 Pro');
let image,packet,expected;
if(action==='restore'){
  image=smkLogicalImage(stock).raw;packet=original.bytes;expected=original.product;
}else{
  const manifest=JSON.parse(await fs.readFile(path.join(root,'web/firmware/candidate.json'),'utf8'));
  if(manifest.release_channel!=='public-beta'||manifest.update_product!=='SMK-37_942'||manifest.image!=='melodee-smk-1.1.0-b1-dc8-candidate.bin'||manifest.sha256!=='5ff8fe4936b8a08175cd80bd6930efbb039f8efae38191034bba11ddf41531ff')throw Error('Unrecognized Melodee public release');
  const app=new Uint8Array(await fs.readFile(path.join(root,'web/firmware',manifest.image)));
  const built=await buildSmkPublicPackage(stock,app,{localCandidate:true,product:manifest.update_product,version:manifest.update_version,bytes:manifest.bytes,sha256:manifest.sha256});
  if(built.stockModel!=='SMK-37 Pro')throw Error('Incorrect physical model');
  image=built.image;packet=built.bytes;expected=built.product;
}
await fs.mkdir(out,{recursive:true});
await fs.writeFile(path.join(out,'image.bin'),image);
await fs.writeFile(path.join(out,'packet.bin'),packet);
await fs.writeFile(path.join(out,'verified.json'),JSON.stringify({model:'SMK-37 Pro',expected,image:{bytes:image.length,sha256:await sha256hex(image)},packet:{bytes:packet.length,sha256:await sha256hex(packet)}}));
