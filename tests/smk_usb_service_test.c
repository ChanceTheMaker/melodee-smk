/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#define FELUCCA_FLASH 1
static uint8_t frame[32], ms_setup_locked;
static uint32_t length;
static int ready,done,ed_calls,inject;
static const uint8_t smk37_update_identity_wire[]={0};
static int ota_frame_get(const uint8_t **p,uint32_t *n){if(!ready)return 0;*p=frame;*n=length;return 1;}
static void ota_frame_done(void){done++;ready=0;}
static void ota_wire_send(const uint8_t *p,uint32_t n){(void)p;(void)n;}
static int melodee_storage_service(const uint8_t *p,uint32_t n){return n>=5&&p[0]==125&&p[1]=='M';}
static void ed_service(void){
 ed_calls++;
 if(ready&&length>=3&&frame[0]==125&&frame[1]=='F'&&frame[2]=='L')ota_frame_done();
 /* A new packet arrives after the editor checked its queue. */
 if(inject){frame[0]=125;frame[1]='M';frame[2]='E';frame[3]='L';frame[4]=1;length=6;ready=1;}
}
#include "../firmware/src/melodee_smk_usb_service.h"
static void reset(void){ready=done=ed_calls=inject=ms_setup_locked=0;length=0;}
int main(void){
 reset();ms_setup_locked=1;inject=1;melodee_usb_service();assert(!ready&&ed_calls==0);
 reset();inject=1;melodee_usb_service();assert(ready&&done==0);
 reset();ready=1;length=4;memcpy(frame,"\x7d" "FLx",4);inject=1;melodee_usb_service();assert(ready&&done==1);
 reset();ready=1;length=1;frame[0]=42;melodee_usb_service();assert(!ready&&done==1);
 reset();ready=1;length=6;memcpy(frame,"\x7d" "MELxx",6);melodee_usb_service();assert(!ready&&done==1&&ed_calls==0);
 reset();ready=1;ms_setup_locked=1;length=4;memcpy(frame,"\x7d" "FLx",4);melodee_usb_service();assert(!ready&&done==1&&ed_calls==0);
 puts("SMK USB service: late-arriving requests preserved; unknown/storage/locked frames consumed");
}
