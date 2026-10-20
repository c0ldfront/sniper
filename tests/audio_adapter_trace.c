/* Production adapter against deterministic COM device; no audio server. */
#include "../platform/audio_player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {IDirectSoundBuffer iface;int id;DWORD frequency,bytes;unsigned char *data;int owns;DWORD status;} Buffer;
static IDirectSoundBufferVtbl bv;
static IDirectSoundVtbl dv;
static IDirectSound device={&dv};
static Buffer buffers[256];static int used,scenario,uploads;
static unsigned hash(const void *data,size_t n){unsigned h=2166136261u;const unsigned char *p=data;while(n--)h=(h^*p++)*16777619u;return h;}
static HRESULT WINAPI coop(IDirectSound *p,HWND w,DWORD level){(void)p;(void)w;printf("coop %lu\n",(unsigned long)level);return 0;}
static HRESULT WINAPI create(IDirectSound *p,LPCDSBUFFERDESC d,LPDIRECTSOUNDBUFFER *out,LPUNKNOWN unk){(void)p;(void)unk;Buffer *b=&buffers[used];b->id=++used;b->iface.lpVtbl=&bv;b->frequency=d->lpwfxFormat->nSamplesPerSec;b->bytes=d->dwBufferBytes;b->data=calloc(b->bytes,1);b->owns=1;*out=&b->iface;printf("create %d %lu %lu ",b->id,(unsigned long)d->dwFlags,(unsigned long)b->bytes);for(int i=0;i<18;i++)printf("%02x",((const unsigned char *)d->lpwfxFormat)[i]);puts("");return 0;}
static HRESULT WINAPI duplicate(IDirectSound *p,LPDIRECTSOUNDBUFFER orig,LPDIRECTSOUNDBUFFER *out){(void)p;Buffer *a=(Buffer *)orig,*b=&buffers[used];if(scenario==1){printf("duplicate_fail %d\n",a->id);return E_FAIL;}*b=*a;b->id=++used;b->owns=0;*out=&b->iface;printf("duplicate %d %d\n",a->id,b->id);return 0;}
static ULONG WINAPI release_buffer(IDirectSoundBuffer *p){Buffer *b=(Buffer *)p;printf("release %d\n",b->id);if(b->owns){free(b->data);b->data=NULL;}return 0;}
static ULONG WINAPI release_device(IDirectSound *p){(void)p;puts("release_device");return 0;}
static HRESULT WINAPI lock(IDirectSoundBuffer *p,DWORD at,DWORD bytes,void **a,DWORD *na,void **b,DWORD *nb,DWORD flags){Buffer *v=(Buffer *)p;printf("lock %d %lu %lu %lu\n",v->id,(unsigned long)at,(unsigned long)bytes,(unsigned long)flags);if(scenario==2&&v->id==1)return E_FAIL;*na=bytes/2;*nb=bytes-*na;*a=v->data;*b=v->data+*na;return 0;}
static HRESULT WINAPI unlock(IDirectSoundBuffer *p,void *a,DWORD na,void *b,DWORD nb){Buffer *v=(Buffer *)p;(void)a;(void)b;char name[96];snprintf(name,sizeof name,"production-buffer-%d-%d.bin",scenario,uploads++);FILE *file=fopen(name,"wb");if(!file)exit(2);if(fwrite(v->data,1,v->bytes,file)!=v->bytes)exit(3);fclose(file);printf("unlock %d %lu %lu %u\n",v->id,(unsigned long)na,(unsigned long)nb,hash(v->data,v->bytes));return 0;}
static HRESULT WINAPI getfreq(IDirectSoundBuffer *p,DWORD *value){Buffer *b=(Buffer *)p;*value=b->frequency;printf("get_frequency %d %lu\n",b->id,(unsigned long)*value);return 0;}
static HRESULT WINAPI status(IDirectSoundBuffer *p,DWORD *value){Buffer *b=(Buffer *)p;*value=b->status;printf("status %d %lu\n",b->id,(unsigned long)*value);return scenario==3&&b->id%3==0?E_FAIL:0;}
static HRESULT WINAPI volume(IDirectSoundBuffer *p,LONG x){printf("volume %d %ld\n",((Buffer *)p)->id,(long)x);return scenario==4?E_FAIL:0;}
static HRESULT WINAPI pan(IDirectSoundBuffer *p,LONG x){printf("pan %d %ld\n",((Buffer *)p)->id,(long)x);return scenario==4?E_FAIL:0;}
static HRESULT WINAPI position(IDirectSoundBuffer *p,DWORD x){printf("position %d %lu\n",((Buffer *)p)->id,(unsigned long)x);return scenario==4?E_FAIL:0;}
static HRESULT WINAPI frequency(IDirectSoundBuffer *p,DWORD x){printf("frequency %d %lu\n",((Buffer *)p)->id,(unsigned long)x);return scenario==4?E_FAIL:0;}
static HRESULT WINAPI play(IDirectSoundBuffer *p,DWORD a,DWORD b,DWORD flags){Buffer *v=(Buffer *)p;if(scenario!=4)v->status=1|(flags?4:0);printf("play %d %lu %lu %lu\n",v->id,(unsigned long)a,(unsigned long)b,(unsigned long)flags);return scenario==4?E_FAIL:0;}
static HRESULT WINAPI stop(IDirectSoundBuffer *p){Buffer *v=(Buffer *)p;v->status=0;printf("stop %d\n",v->id);return 0;}
static HRESULT WINAPI restore(IDirectSoundBuffer *p){Buffer *v=(Buffer *)p;if(scenario!=4)v->status=0;printf("restore %d\n",v->id);return scenario==4?E_FAIL:0;}
static HRESULT fake_create(LPCGUID g,LPDIRECTSOUND *out,LPUNKNOWN u){(void)g;(void)u;if(scenario==5){puts("device_create_fail");return E_FAIL;}puts("device_create");*out=&device;return 0;}
#define DirectSoundCreate fake_create
#include "../platform/audio_player.c"
int main(int argc,char **argv){
 scenario=argc>1?atoi(argv[1]):0;
 dv.SetCooperativeLevel=coop;dv.CreateSoundBuffer=create;dv.DuplicateSoundBuffer=duplicate;dv.Release=release_device;
 bv.Release=release_buffer;bv.Lock=lock;bv.Unlock=unlock;bv.GetFrequency=getfreq;bv.GetStatus=status;bv.SetVolume=volume;bv.SetPan=pan;bv.SetCurrentPosition=position;bv.SetFrequency=frequency;bv.Play=play;bv.Stop=stop;bv.Restore=restore;
 SniperAudioPlayer p;if(FAILED(sniper_audio_open(&p,(HWND)87,GetModuleHandleA(NULL))))return 0;
 puts("MARK events");SniperAudioRequest r={0,0,1,0};sniper_audio_play(&p,127,&r);sniper_audio_stop(&p,169);sniper_audio_play(&p,127,&r);sniper_audio_play(&p,169,&r);
 sniper_audio_position(1100,2200,1000000,2000000,1,640,&r);r.pitch=sniper_audio_pitch(13,100);sniper_audio_play(&p,134,&r);sniper_audio_play(&p,121,&r);
 /* Lost selected voice must restore then refill first shared buffer. */
 p.sounds[9].next=p.sounds[9].count>2?2:0;((Buffer *)p.sounds[9].buffer[p.sounds[9].next])->status=2;sniper_audio_play(&p,134,&r);
 puts("MARK message");HINSTANCE inst=GetModuleHandleA(NULL);HRSRC resource=FindResourceA(inst,MAKEINTRESOURCEA(171),"WAVE");const uint8_t *raw=LockResource(LoadResource(inst,resource));SniperAudioVoice message;
 sniper_audio_memory_wave(&p,&message,raw,SizeofResource(inst,resource));r=(SniperAudioRequest){0};sniper_audio_voice_play(&p,&message,&r);
 puts("MARK inactive");sniper_audio_set_active(&p,0);sniper_audio_play(&p,134,&r);sniper_audio_voice_play(&p,&message,&r);
 sniper_audio_set_active(&p,1);sniper_audio_voice_play(&p,&message,&r);
 puts("MARK shutdown");sniper_audio_stop_all(&p);sniper_audio_voice_close(&message);sniper_audio_close(&p);return 0;
}
