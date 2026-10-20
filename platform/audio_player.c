#include "audio_player.h"
#include <string.h>
static HRESULT fill(SniperAudioVoice *v){void *a,*b;DWORD na,nb;HRESULT h=IDirectSoundBuffer_Lock(v->buffer[0],0,v->bytes,&a,&na,&b,&nb,0);if(FAILED(h))return h;memcpy(a,v->pcm,na);if(nb)memcpy(b,v->pcm+na,nb);return IDirectSoundBuffer_Unlock(v->buffer[0],a,na,b,nb);}
void sniper_audio_close(SniperAudioPlayer *p){if(!p)return;for(int i=0;i<17;i++)for(int j=0;j<16;j++)if(p->sounds[i].buffer[j])IDirectSoundBuffer_Release(p->sounds[i].buffer[j]);if(p->device)IDirectSound_Release(p->device);memset(p,0,sizeof *p);}
HRESULT sniper_audio_open(SniperAudioPlayer *p,HWND window,HINSTANCE instance){
 memset(p,0,sizeof *p);HRESULT h=DirectSoundCreate(NULL,&p->device,NULL);if(FAILED(h))return h;
 h=IDirectSound_SetCooperativeLevel(p->device,window,DSSCL_NORMAL);if(FAILED(h)){sniper_audio_close(p);return h;}
 for(int i=0;i<17;i++){
  HRSRC resource=FindResourceA(instance,MAKEINTRESOURCEA(sniper_audio_assets[i].resource),"WAVE");if(!resource)continue;
  DWORD size=SizeofResource(instance,resource);const unsigned char *raw=LockResource(LoadResource(instance,resource));
  sniper_audio_voice_load(p,&p->sounds[i],raw,size,sniper_audio_assets[i].voices);

 }
 p->enabled=1;p->active=1;return DS_OK;
}
static SniperAudioVoice *voice(SniperAudioPlayer *p,int resource){for(int i=0;i<17;i++)if(sniper_audio_assets[i].resource==resource)return &p->sounds[i];return NULL;}
void sniper_audio_voice_play(SniperAudioPlayer *p,SniperAudioVoice *v,const SniperAudioRequest *r){
 if(!p->device||!p->enabled||!p->active||!v||!v->count)return;
 IDirectSoundBuffer *b=v->buffer[v->next];DWORD status=0;HRESULT h=IDirectSoundBuffer_GetStatus(b,&status);
 if(!sniper_audio_should_play(1,p->enabled&&p->active,v->count,SUCCEEDED(h),status))return;
 if((status&DSBSTATUS_BUFFERLOST)&&v->pcm){IDirectSoundBuffer_Restore(b);fill(v);}
 IDirectSoundBuffer_SetVolume(b,r->volume);IDirectSoundBuffer_SetPan(b,r->pan);IDirectSoundBuffer_SetCurrentPosition(b,0);
 IDirectSoundBuffer_SetFrequency(b,(DWORD)sniper_audio_frequency(v->frequency,r->pitch));
 IDirectSoundBuffer_Play(b,0,0,r->loop?DSBPLAY_LOOPING:0);v->next=(v->next+1)%v->count;
}
void sniper_audio_play(SniperAudioPlayer *p,int resource,const SniperAudioRequest *r){sniper_audio_voice_play(p,voice(p,resource),r);}

void sniper_audio_stop(SniperAudioPlayer *p,int resource){SniperAudioVoice *v=voice(p,resource);if(p->device&&v)for(int j=0;j<v->count;j++)IDirectSoundBuffer_Stop(v->buffer[j]);}
void sniper_audio_stop_all(SniperAudioPlayer *p){for(int i=0;i<17;i++)sniper_audio_stop(p,sniper_audio_assets[i].resource);}

void sniper_audio_enable(SniperAudioPlayer *p,int enabled){
 if(!p->device)return;
 if(!enabled&&p->enabled)sniper_audio_stop_all(p);
 p->enabled=enabled;
}

void sniper_audio_voice_close(SniperAudioVoice *v){
 if(!v)return;
 for(int j=0;j<16;j++)if(v->buffer[j])IDirectSoundBuffer_Release(v->buffer[j]);
 memset(v,0,sizeof *v);
}
HRESULT sniper_audio_voice_load(SniperAudioPlayer *p,SniperAudioVoice *v,const uint8_t *raw,size_t size,int count){
 /* Caller supplies an empty voice; resource bytes remain borrowed for restore. */
 memset(v,0,sizeof *v);
 if(!p->device)return DSERR_UNINITIALIZED;
 SniperAudioWave wave;
 if(!sniper_audio_wave(raw,size,&wave)||!wave.has_format||!wave.pcm||!wave.bytes)return DSERR_BADFORMAT;
 WAVEFORMATEX fmt;memcpy(&fmt,wave.format,18);
 if(fmt.wFormatTag!=1||!fmt.nChannels)return DSERR_BADFORMAT;
 v->pcm=wave.pcm;v->bytes=wave.bytes;
 DSBUFFERDESC desc={0};desc.dwSize=sizeof desc;desc.dwFlags=0xe2;desc.dwBufferBytes=v->bytes;desc.lpwfxFormat=&fmt;
 HRESULT h=IDirectSound_CreateSoundBuffer(p->device,&desc,&v->buffer[0],NULL);
 if(FAILED(h))return h;
 if(count<1)count=1;
 if(count>16)count=16;
 v->count=1;for(int j=1;j<count;j++){if(FAILED(IDirectSound_DuplicateSoundBuffer(p->device,v->buffer[0],&v->buffer[j])))break;v->count++;}
 fill(v);DWORD frequency=fmt.nSamplesPerSec;IDirectSoundBuffer_GetFrequency(v->buffer[0],&frequency);v->frequency=(int32_t)frequency;
 return DS_OK;
}
HRESULT sniper_audio_memory_wave(SniperAudioPlayer *p,SniperAudioVoice *v,const uint8_t *raw,size_t size){
 HRESULT h=sniper_audio_voice_load(p,v,raw,size,1);
 /* Original40a930 frees PCM after upload,resource ID=-1 suppresses restore. */
 v->pcm=NULL;return h;
}

void sniper_audio_set_active(SniperAudioPlayer *p,int active){
 if(!p->device)return;
 p->active=active;
 if(!active)sniper_audio_stop_all(p);
}
