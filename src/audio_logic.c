#include "audio_logic.h"
const SniperAudioAsset sniper_audio_assets[17]={
 {171,1,0x4218e4},{148,1,0x4218e8},{121,4,0x4218ec},{123,4,0x4218f0},
 {124,4,0x4218f4},{177,2,0x4218f8},{125,1,0x4218fc},{127,1,0x421900},
 {169,1,0x421904},{134,4,0x421908},{143,2,0x42190c},{162,4,0x428190},
 {164,4,0x428194},{165,4,0x428198},{166,4,0x428168},{167,4,0x42816c},{168,4,0x428170}};
static int32_t add(int32_t a,int32_t b){return (int32_t)((uint32_t)a+(uint32_t)b);}
static int32_t mul(int32_t a,int32_t b){return (int32_t)((uint32_t)a*(uint32_t)b);}
static int32_t abs32(int32_t a){return a<0?(int32_t)(0u-(uint32_t)a):a;}
int sniper_audio_position(int32_t x,int32_t y,int32_t lx,int32_t ly,int present,int32_t width,SniperAudioRequest *r){
 if(!present||(x<0&&y<0)||width/2==0)return 0;
 int32_t dx=add(x,lx/-1000),dy=add(y,ly/-1000);
 int32_t d=abs32(dx)>abs32(dy)?abs32(dx):abs32(dy);
 if(d>800)return 0;
 r->volume=(int32_t)(0u-(uint32_t)mul(d,4));if(r->volume<=-10000)r->volume=-10000;
 r->pan=(int32_t)((int64_t)mul(dx,10000)/(width/2));r->loop=0;r->pitch=0;return 1;
}
int32_t sniper_audio_pitch(int32_t random,int32_t spread){int32_t div=mul(spread,2);return div?add((int32_t)((int64_t)random%div),(int32_t)(0u-(uint32_t)spread)):0;}
int32_t sniper_audio_frequency(int32_t base,int32_t pitch){return add(mul(base,pitch)/10000,base);}
int sniper_audio_should_play(int available,int enabled,int count,int ok,uint32_t status){return available&&enabled&&count&&ok&&((status&5)!=5);}

static uint32_t wave_u32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
int sniper_audio_wave(const uint8_t *raw,size_t size,SniperAudioWave *out){
    if(!raw||!out||size<12||wave_u32(raw)!=0x46464952u||wave_u32(raw+8)!=0x45564157u)return 0;
    uint64_t end=(uint64_t)wave_u32(raw+4)+8;
    if(end>size||end<12)return 0;
    SniperAudioWave result={{0},0,0,0};
    for(size_t at=12;at<end;){
        if(end-at<8)return 0;
        uint32_t kind=wave_u32(raw+at),n=wave_u32(raw+at+4);at+=8;
        if(n>end-at)return 0;
        if(kind==0x20746d66u){
            if(n<16||size-at<18)return 0;
            for(unsigned i=0;i<18;i++)result.format[i]=raw[at+i];
            result.has_format=1;
        }else if(kind==0x61746164u){result.pcm=raw+at;result.bytes=n;}
        at+=(size_t)n+(n&1u);
    }
    *out=result;return 1;
}
