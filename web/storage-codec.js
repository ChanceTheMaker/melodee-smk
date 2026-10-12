// SPDX-License-Identifier: GPL-3.0-only
export const prefix=[125,77,69,76,1];
export const u32=n=>Array.from({length:5},(_,i)=>(n>>>i*7)&127);
export const read32=(p,o=0)=>((p[o]|p[o+1]<<7|p[o+2]<<14|p[o+3]<<21|p[o+4]<<28)>>>0);
export function crc32(p){let c=0xffffffff;for(const b of p){c^=b;for(let j=0;j<8;j++)c=(c>>>1)^((-(c&1))&0xedb88320);}return (~c)>>>0;}
export function unpack(p,n){const out=[];for(let i=0;i<p.length;){const mask=p[i++],k=Math.min(7,p.length-i);if(!k||mask>>>k)throw Error('Malformed backup packet');for(let j=0;j<k;j++)out.push(p[i++]|((mask>>>j)&1)<<7);}if(out.length!==n)throw Error('Short backup packet');return Uint8Array.from(out);}
