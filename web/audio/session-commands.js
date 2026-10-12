// SPDX-License-Identifier: GPL-3.0-only
// Editor frames include F0 / manufacturer bytes / opcode / arguments / F7.
// Read-only polling must not keep postponing the pending autosave.
export function changesSession(bytes) {
  const cmd=bytes[4], n=bytes.length-6, x=bytes[5], y=bytes[6];
  if([3,7,8,12,13,14,18,19,20,21,35,38,67,69,71,76].includes(cmd))return true;
  if(cmd===9)return x!==2;
  if(cmd===27)return n>0;
  if(cmd===28)return n>1;
  if(cmd===30||cmd===31)return n>2;
  if(cmd===33||cmd===74)return x!==0;
  if(cmd===64)return n>1&&y!==0;
  if(cmd===73)return y!==0;
  if(cmd===78)return y>=2&&y<=6;
  if(cmd===95)return x===1||x===2;
  return false;
}
