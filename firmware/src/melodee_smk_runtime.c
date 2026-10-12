/* SPDX-License-Identifier: GPL-3.0-only */
/* Melodee 1.1 musical/UI core with the independently measured SMK panel. */
#include <stdint.h>
#define MELODEE_SMK 1
#define MELODEE_DUAL_CORE 0
#define MELODEE_CACHE_RAM 0
#define MELODEE_CORE1_TEST 0
#define MELODEE_UART 0
#define MELODEE_CDC 0
#define MELODEE_USB_AUDIO 0
#define MELODEE_OTA 1
#ifdef MELODEE_SMK_HARDWARE
#define MELODEE_FLASH 1 /* native bounded caches; backing store deliberately refuses writes */
#else
#define MELODEE_FLASH 0
static volatile uint32_t fm1_ms;
static uint32_t fm1_ticks(void){return fm1_ms*24000u;}
static void fm1_irq_off(void){}
static void fm1_irq_on(void){}
static void fm1_wdt_feed(void){}
static void fm1_delay_ms(uint32_t ms){fm1_ms+=ms;}
#endif
#ifndef MELODEE_VERSION
#define MELODEE_VERSION "SMK 1.1.0-dev1"
#endif
#define MELODEE_ID "SMK-37_940"
#define FM1_TICKS_PER_US 24u
#include "melodee_tables.h"
#include "libc.c"
static struct {volatile uint32_t notes,buttons;} fm1_in;
static const uint8_t empty_sample[512];
#define SMP_USER_XIP(k) empty_sample
#include "core.h"
#include "engines.c"
#include "params.c"
#include "mod.c"
#include "voice.c"
#include "slicer.c"
#include "fx.c"
#include "usb.c"
#include "song_chain.c"
static int smk_local_held(const track_t*,uint32_t);
#include "seq.c"
static int32_t smk_enc[7];
static int32_t fm1_enc_take(uint32_t e){int32_t n=smk_enc[e%7];smk_enc[e%7]=0;return n;}
static uint32_t smk_edges,smk_notes_seen;
static uint32_t fm1_input_edges(int x){(void)x;uint32_t n=smk_edges;smk_edges=0;return n;}
static uint32_t fm1_input_note_edges(void){uint32_t n=fm1_in.notes&~smk_notes_seen;smk_notes_seen=fm1_in.notes;return n;}
#define FM1_NCOL 16u
static const int8_t FM1_KEYMAP[5][16]={
 {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
 {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
 {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31},
 {32,33,34,35,36,37,38,39,40,-1,-1,-1,-1,-1,-1,-1},
 {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};
static uint8_t fm1_led[16],fm1_led_dim[3][16],fm1_led_dim_mask[3];
#define SCOPE_N 512u
static int16_t scope_buf[SCOPE_N];static uint32_t scope_w;
static struct {uint32_t stage;} melodee_dbg;
#define UI_LCD_POWER(s) ((void)(s))
#define UI_LCD_WAKE() ((void)0)
#include "gfx.c"
#include "panel.c"
#include "ui.c"
#include "icons.c"
#include "ui_graph.c"
#include "ui_draw.c"
#include "ui_menu.c"
#include "ui_input.c"
#include "ui_layer.c"
/* No FM-1 flash geometry, erase/program driver, or calibration writer is linked. */
static uint32_t flash_ok;
#ifdef MELODEE_SMK_HARDWARE
static uint32_t irq_save(void){fm1_irq_off();return 0;}
static void irq_restore(uint32_t f){(void)f;fm1_irq_on();}
#define ED_BK_FLASH_PTR(off) ((const uint8_t*)0)
#endif
static int st_read(uint32_t off,void*p,uint32_t n){(void)off;(void)p;(void)n;return -1;}
static int st_prog(uint32_t off,const void*p,uint32_t n){(void)off;(void)p;(void)n;return -1;}
static int st_erase(uint32_t off){(void)off;return -1;}
static int fl_erase4k(uint32_t off,uint32_t*took){(void)off;if(took)*took=0;return -1;}
static int fl_write(uint32_t off,const void*p,uint32_t n){(void)off;(void)p;(void)n;return -1;}
static void fl_inval(uint32_t off,uint32_t n){(void)off;(void)n;}
static void audio_silence(void){}
#include "storage.c"
#include "upreset.c"
#include "project.c"
#include "cz_store.c"
#include "fm6_store.c"
#include "editor.c"
#include "smk37_pad_feedback.h"
static smk37_pad_feedback_t smk_pad_feedback;
static uint8_t smk_knob_bank,smk_fader_bank,smk_sustain;
static uint64_t smk_keys;
static uint16_t smk_pads;
static uint8_t smk_note[53],smk_owner[53],smk_active[53],smk_ui_key[37],smk_deferred[53],smk_chord_n[53],smk_chord[53][CHORD_MAX];
static int smk_local_held(const track_t*t,uint32_t note){for(unsigned k=0;k<53;k++)if(smk_owner[k]==trk_index(t))for(unsigned i=0;i<smk_chord_n[k];i++)if(smk_chord[k][i]==note)return 1;return 0;}
static void smk_note_release(unsigned id){unsigned t=smk_owner[id],count=smk_chord_n[id];smk_chord_n[id]=smk_active[id]=smk_deferred[id]=0;for(unsigned i=0;i<count;i++){unsigned n=smk_chord[id][i];if(midi_local_held(&trk[t],n))continue;input_off(&trk[t],n);midi_out_event(8u|((0x80u|trk_midi_ch(t))<<8)|(n<<16));}}
static uint32_t smk_status[16];
static const int8_t smk_buttons[18]={-1,-1,B_OCTDN,B_OCTUP,B_OCTDN,B_OCTUP,B_ARP,B_LFO,B_SCL,B_ENV,B_GLO,B_EDIT,B_HOME,B_SAVE,B_FX,B_SEQ,B_PLAY,B_REC};
static void melodee_smk_init(void){
 chain_defaults(&chain_config);pattern_init();
 for(unsigned i=0;i<G_COUNT;i++)song.g[i]=GP[i].def;
 fm6_init();cz_init();undo_depth++;
 for(unsigned i=0;i<NTRK;i++){track_t*t=&trk[i];track_defaults(t);set_engine_of(t,TRK_DEF[i][0]);apply_preset_to(t,TRK_DEF[i][1]);t->engine=t->eng_req;track_defaults_steps(t);pat_sig[i]=steps_sig(t);}
 undo_depth--;song.master_q12=2048;panel=PANEL_DEFAULT;settings_init();ui.home=1;ui.force=1;panic_req=0;
}
static void melodee_smk_midi(unsigned st,unsigned a,unsigned b){
 if(st<128||st>=240||a>=128||b>=128)return;
 midi_enqueue((st>>4)|(st<<8)|(a<<16)|(b<<24),1);
}
static void melodee_smk_note(unsigned id,unsigned velocity){
 if(id>=53)return;
 if(!velocity){
  if(id<37&&smk_ui_key[id]){fm1_in.notes&=~(1u<<(id-5));smk_ui_key[id]=0;return;}
  if(smk_active[id]){if(smk_sustain&&!drum_track(&trk[smk_owner[id]])){smk_active[id]=0;smk_deferred[id]=1;}else smk_note_release(id);}return;
 }
 if(smk_active[id])return;
 if(smk_deferred[id])smk_note_release(id);
 if(id>=5&&id<32&&(song.grid||kb_lock||(fm1_in.buttons&kb_mask))){fm1_in.notes|=1u<<(id-5);smk_ui_key[id]=1;return;}
 unsigned t=id>=37?NTRK-1:song.sel%NTRK;
 static const uint8_t pads[16]={36,38,42,46,41,43,45,49,51,37,39,56,54,75,76,77};
 unsigned n=id>=37?pads[id-37]:clamp(48+(int)id+song.octave*12,0,127);
 if(id<37){track_t*track=&trk[t];if(track->p[P_QUANT]==Q_SNAP){if(micro_active(track))n=micro_snap(track,n+track->p[P_TRANS]);else{unsigned mask=scale_mask(track),guard=12;n=clamp((int)n+track->p[P_TRANS],0,127);while(guard--&&!((mask>>((n-track->p[P_ROOT]+120)%12))&1u))n=n? n-1:0;}}else n=scale_map(track,48+(int)id,12*song.octave);}
 if(n==KB_SILENT)return;
 smk_owner[id]=t;smk_note[id]=n;smk_active[id]=1;
 unsigned count=chord_build(&trk[t],n,smk_chord[id]);
 for(unsigned i=0;i<count;i++){unsigned x=smk_chord[id][i];if(midi_local_held(&trk[t],x))continue;input_on(&trk[t],x,velocity);midi_out_event(9u|((0x90u|trk_midi_ch(t))<<8)|(x<<16)|(velocity<<24));}
 smk_chord_n[id]=count;
}
static void melodee_smk_event(unsigned kind,unsigned id,int value){
 if(kind==7){transport_req=value?1:2;return;}
 if(kind==0&&id<18){if(id<2){if(value){if(id==0)smk_knob_bank^=1;else smk_fader_bank^=1;}return;}uint32_t bit=1u<<panel.btn[(unsigned)smk_buttons[id]];if(value){if(!(fm1_in.buttons&bit))smk_edges|=bit;fm1_in.buttons|=bit;}else fm1_in.buttons&=~bit;}
 if(kind==1&&id<8){value=clamp(value,-32,32);if(smk_knob_bank){const unsigned p[8]={P_ATK,P_DEC,P_SUS,P_REL,P_DIST,P_CHOR,P_DLY,P_REV};TSEL->p[p[id]]=clamp(TSEL->p[p[id]]+value,0,127);ui.force=1;return;}if(id==0){song.master_q12=clamp(song.master_q12+value*64,0,4096);ui.force=1;return;}const unsigned role[8]={EN_ALGO,EN_SELECT,EN_PRESET,EN_ALGO,EN_K1,EN_K2,EN_K3,EN_K4};smk_enc[panel.enc[role[id]]]+=value;}
 if(kind==2&&id<37){if(value)smk_keys|=(uint64_t)1<<id;else smk_keys&=~((uint64_t)1<<id);melodee_smk_note(id,clamp(value,0,127));}
 if(kind==3&&id<16){smk37_pad_feedback_event(&smk_pad_feedback,id,value,fm1_ms);if(value)smk_pads|=1u<<id;else smk_pads&=~(1u<<id);melodee_smk_note(37+id,clamp(value,0,127));}
 if(kind==4&&id<4){track_t*t=smk_fader_bank?TSEL:&trk[id];unsigned p=smk_fader_bank?P_ATK+id:P_LEVEL;t->p[p]=clamp(value,0,1023)*127/1023;ui.force=1;}
 if(kind==5){for(unsigned t=0;t<NTRK;t++){unsigned ch=trk_midi_ch(t);if(!id){unsigned bend=clamp(value+8192,0,16383);melodee_smk_midi(0xe0|ch,bend&127,bend>>7);}else melodee_smk_midi(0xb0|ch,1,clamp(value,0,127));}}
 if(kind==6){smk_sustain=!!value;if(!smk_sustain)for(unsigned i=0;i<53;i++)if(smk_deferred[i])smk_note_release(i);for(unsigned t=0;t<NTRK;t++)melodee_smk_midi(0xb0|trk_midi_ch(t),64,value?127:0);}
}
static void melodee_smk_release(void){smk_sustain=0;for(unsigned i=0;i<53;i++){melodee_smk_note(i,0);if(smk_deferred[i])smk_note_release(i);}for(unsigned t=0;t<NTRK;t++){melodee_smk_midi(0xb0|trk_midi_ch(t),64,0);melodee_smk_midi(0xe0|trk_midi_ch(t),0,64);}smk_keys=smk_pads=smk_sustain=fm1_in.notes=fm1_in.buttons=smk_edges=smk_notes_seen=0;memset(smk_enc,0,sizeof smk_enc);memset(&smk_pad_feedback,0,sizeof smk_pad_feedback);panic_req=(1u<<NTRK)-1;}
static void melodee_smk_tick(void){ui_input();ui_leds();ui_draw();}
static uint32_t*melodee_smk_status(void){uint32_t leds=smk_knob_bank|(smk_fader_bank<<1);for(unsigned k=2;k<18;k++){unsigned q=led_pos[panel.btn[(unsigned)smk_buttons[k]]];if(q!=255&&((fm1_led[q>>3]>>(q&7))&1))leds|=1u<<k;}smk_status[0]=leds;smk_status[1]=smk_keys;smk_status[2]=smk_keys>>32;smk_status[3]=smk37_pad_feedback_mask(&smk_pad_feedback,fm1_ms);smk_status[4]=ui.page;smk_status[5]=song.sel;smk_status[6]=song.playing;smk_status[7]=smk_knob_bank;smk_status[8]=smk_fader_bank;smk_status[9]=song.octave+3;smk_status[11]=song.master_q12;for(unsigned i=0;i<4;i++)smk_status[12+i]=(smk_fader_bank?TSEL->p[P_ATK+i]:trk[i].p[P_LEVEL])*1023u/127u;return smk_status;}
