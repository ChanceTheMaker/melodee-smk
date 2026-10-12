/* SPDX-License-Identifier: GPL-3.0-only */
/* SMK append/replace object store. One committed generation per object is
 * retained while its replacement is written into unreferenced sectors.
 * Root header commits last. No erase on boot; explicit provisioning required.
 * Sector tails remain erased because the vendor SPL scans them for OTA records. */
#define MS_SECTORS 95u
#define MS_DATA 3584u
#define MS_ROOT 0x3252534du
#define MS_PART 0x3250534du
#define MS_OWNER 0x324f534du
#define MS_LAYOUT 0x20261012u
#define MS_MARKER 0xfe000u
static uint8_t ms_mounted;
static uint32_t ms_unit;
typedef struct {uint32_t magic,obj,seq,len,crc,next,part,hcrc;} ms_head_t;
static int ms_read(uint32_t,void*,uint32_t);
static int ms_erase(uint32_t);
static int ms_prog(uint32_t,const void*,uint32_t);
static uint32_t ms_sector(uint32_t i){return i<35u?0x9d000u+i*4096u:0xc2000u+(i-35u)*4096u;}
static uint32_t ms_crc_step(uint32_t c,const void *v,uint32_t n){const uint8_t*p=v;while(n--){c^=*p++;for(unsigned j=0;j<8;j++)c=(c>>1)^((0u-(c&1u))&0xedb88320u);}return c;}
static uint32_t st_crc32(const void*p,uint32_t n){return ~ms_crc_step(~0u,p,n);}
static int ms_header(uint32_t s,ms_head_t*h){return s>=MS_SECTORS||ms_read(ms_sector(s),h,sizeof *h)||h->hcrc!=st_crc32(h,28)?-1:0;}
static uint32_t st_capacity(uint32_t o){return o>=OBJ_BANK0&&o<OBJ_CZBANK0?ST_BANK_BASE_MAX+ST_BANK_EXT_MAX:ST_PAYLOAD_MAX;}
static int st_retired(uint32_t o){return o>=OBJ_PROJECT0&&o<OBJ_PROJECT0+4u;}
/* Validate a whole chain before selecting it or protecting it from collection. */
static int ms_chain(uint32_t root,uint32_t obj,ms_head_t*out,uint8_t *used){
 ms_head_t h,first;uint8_t seen[MS_SECTORS]={0},buf[256];uint32_t s=root,pos=0,part=0,crc=~0u;
 if(ms_header(s,&first)||first.magic!=MS_ROOT||first.obj!=obj||obj>=OBJ_COUNT||st_retired(obj)||first.len>st_capacity(obj))return -1;
 do{if(s>=MS_SECTORS||seen[s]||ms_header(s,&h)||h.magic!=(part?MS_PART:MS_ROOT)||h.obj!=obj||h.seq!=first.seq||h.len!=first.len||h.crc!=first.crc||h.part!=part)return -1;
 seen[s]=1;uint32_t n=first.len-pos;if(n>MS_DATA)n=MS_DATA;
 for(uint32_t off=0;off<n;){uint32_t k=n-off;if(k>sizeof buf)k=sizeof buf;if(ms_read(ms_sector(s)+256+off,buf,k))return -1;crc=ms_crc_step(crc,buf,k);off+=k;}
 pos+=n;++part;if(pos==first.len){if(h.next!=UINT32_MAX)return -1;break;}s=h.next;
 }while(part<MS_SECTORS);
 if(pos!=first.len||~crc!=first.crc)return -1;
 if(used)for(unsigned i=0;i<MS_SECTORS;i++)used[i]|=seen[i];if(out)*out=first;return 0;
}
static int st_current(uint32_t obj,st_hdr_t*out){
 if(!ms_mounted||obj>=OBJ_COUNT||st_retired(obj))return -1;
 int found=-1;ms_head_t best={0},h;
 for(unsigned s=0;s<MS_SECTORS;s++)if(!ms_header(s,&h)&&h.magic==MS_ROOT&&h.obj==obj&&
  (found<0||(h.seq!=best.seq&&h.seq-best.seq<0x80000000u))&&!ms_chain(s,obj,&h,0)){best=h;found=(int)s;}
 if(found>=0&&out){memset(out,0,sizeof *out);out->magic=ST_MAGIC;out->type=obj;out->slot=found;out->seq=best.seq;out->len=best.len;out->crc=best.crc;out->hcrc=st_crc32(out,sizeof *out-4u);}return found;
}
static uint32_t st_sector(uint32_t obj,uint32_t copy){(void)obj;return copy<MS_SECTORS?ms_sector(copy):UINT32_MAX;}
static int st_read_range(uint32_t obj,uint32_t copy,uint32_t off,void*dst,uint32_t n){
 ms_head_t first,h;uint8_t*p=dst;if(!ms_mounted||ms_header(copy,&first)||first.magic!=MS_ROOT||first.obj!=obj||off>first.len||n>first.len-off)return -1;
 unsigned s=copy,part=0;while(n){if(part>=MS_SECTORS||ms_header(s,&h)||h.obj!=obj||h.seq!=first.seq||h.part!=part)return -1;
 if(off>=MS_DATA){off-=MS_DATA;s=h.next;++part;continue;}uint32_t k=MS_DATA-off;if(k>n)k=n;
 if(ms_read(ms_sector(s)+256+off,p,k))return -1;p+=k;n-=k;off=0;s=h.next;++part;}return 0;
}
static int st_load(uint32_t obj,void*dst,uint32_t max){st_hdr_t h;int c=st_current(obj,&h);if(c<0||h.len>max||st_read_range(obj,c,0,dst,h.len))return -1;return (int)h.len;}
static int st_save(uint32_t obj,const void*src,uint32_t len){
 if(!ms_mounted||obj>=OBJ_COUNT||st_retired(obj)||len>st_capacity(obj)||(!src&&len))return -1;
 uint8_t used[MS_SECTORS]={0},slots[MS_SECTORS],verify[256];st_hdr_t old;uint32_t seq=1;
 for(unsigned o=0;o<OBJ_COUNT;o++){int c=st_current(o,&old);if(c>=0){if(ms_chain(c,o,0,used))return -1;if(o==obj)seq=old.seq+1u;}}
 unsigned need=(len+MS_DATA-1u)/MS_DATA;if(!need)need=1;unsigned count=0;
 for(unsigned s=0;s<MS_SECTORS&&count<need;s++)if(!used[s])slots[count++]=(uint8_t)s;
 if(count!=need)return -2; /* leave current objects intact when full */
 uint32_t crc=st_crc32(src,len);const uint8_t*p=src;
 for(unsigned part=0;part<need;part++){uint32_t base=ms_sector(slots[part]),n=len-part*MS_DATA;if(n>MS_DATA)n=MS_DATA;
 if(ms_erase(base))return -3;
 for(uint32_t off=0;off<n;){uint32_t k=n-off;if(k>256)k=256;if(ms_prog(base+256+off,p+part*MS_DATA+off,k)||ms_read(base+256+off,verify,k)||memcmp(verify,p+part*MS_DATA+off,k))return -3;off+=k;}
 if(part){ms_head_t h={MS_PART,obj,seq,len,crc,part+1<need?slots[part+1]:UINT32_MAX,part,0};h.hcrc=st_crc32(&h,28);if(ms_prog(base,&h,sizeof h))return -3;}}
 ms_head_t h={MS_ROOT,obj,seq,len,crc,need>1?slots[1]:UINT32_MAX,0,0};h.hcrc=st_crc32(&h,28);
 if(ms_prog(ms_sector(slots[0]),&h,sizeof h)||ms_chain(slots[0],obj,0,0))return -3;return 0;
}
static int ms_mount(uint32_t unit){ms_head_t h;ms_mounted=0;ms_unit=unit;if(ms_read(MS_MARKER,&h,sizeof h)||h.magic!=MS_OWNER||h.obj!=MS_LAYOUT||h.seq!=unit||h.len!=MS_SECTORS||h.hcrc!=st_crc32(&h,28))return 0;ms_mounted=1;return 1;}
/* Caller must obtain explicit consent and a verified backup before this call. */
static int ms_format(uint32_t unit){
 ms_mounted=0;if(ms_erase(MS_MARKER))return -1;
 for(unsigned i=0;i<MS_SECTORS;i++)if(ms_erase(ms_sector(i)))return -1;
 ms_head_t h={MS_OWNER,MS_LAYOUT,unit,MS_SECTORS,0,UINT32_MAX,0,0};h.hcrc=st_crc32(&h,28);
 if(ms_prog(MS_MARKER,&h,sizeof h))return -1;return ms_mount(unit)?0:-1;
}
