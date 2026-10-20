#include "../platform/audio_player.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){printf("FAIL line%d\n",__LINE__);return 1;}}while(0)
int main(void){
 HWND w=CreateWindowExA(0,"STATIC","audio-test",WS_OVERLAPPEDWINDOW,0,0,320,200,NULL,NULL,GetModuleHandleA(NULL),NULL);CHECK(w);
 SniperAudioPlayer p;HRESULT h=sniper_audio_open(&p,w,GetModuleHandleA(NULL));printf("open=%08lx\n",(unsigned long)h);CHECK(SUCCEEDED(h));
 int voices=0;
 for(int i=0;i<17;i++){
  SniperAudioVoice *v=&p.sounds[i];CHECK(v->count==sniper_audio_assets[i].voices);voices+=v->count;
  void *a,*b;DWORD na,nb;CHECK(SUCCEEDED(IDirectSoundBuffer_Lock(v->buffer[0],0,v->bytes,&a,&na,&b,&nb,0)));CHECK(!memcmp(a,v->pcm,na));if(nb)CHECK(!memcmp(b,v->pcm+na,nb));IDirectSoundBuffer_Unlock(v->buffer[0],a,na,b,nb);
  SniperAudioRequest req={-1234,2000,1,100};sniper_audio_play(&p,sniper_audio_assets[i].resource,&req);DWORD status;CHECK(SUCCEEDED(IDirectSoundBuffer_GetStatus(v->buffer[0],&status)));CHECK((status&5)==5);
  LONG volume,pan;DWORD frequency;IDirectSoundBuffer_GetVolume(v->buffer[0],&volume);IDirectSoundBuffer_GetPan(v->buffer[0],&pan);IDirectSoundBuffer_GetFrequency(v->buffer[0],&frequency);
  CHECK(volume==-1234&&pan==2000&&frequency==(DWORD)sniper_audio_frequency(v->frequency,100));
  int next=v->next;sniper_audio_enable(&p,0);IDirectSoundBuffer_GetStatus(v->buffer[0],&status);CHECK(!(status&1));sniper_audio_play(&p,sniper_audio_assets[i].resource,&req);CHECK(next==v->next);sniper_audio_enable(&p,1);
  HRSRC resource=FindResourceA(GetModuleHandleA(NULL),MAKEINTRESOURCEA(sniper_audio_assets[i].resource),"WAVE");
  DWORD size=SizeofResource(GetModuleHandleA(NULL),resource);const void *raw=LockResource(LoadResource(GetModuleHandleA(NULL),resource));
  unsigned char *copy=malloc(size);CHECK(copy);memcpy(copy,raw,size);SniperAudioVoice message;
  CHECK(SUCCEEDED(sniper_audio_memory_wave(&p,&message,copy,size)));memset(copy,0,size);free(copy);
  CHECK(message.count==1&&!message.pcm&&message.bytes==v->bytes);
  CHECK(SUCCEEDED(IDirectSoundBuffer_Lock(message.buffer[0],0,message.bytes,&a,&na,&b,&nb,0)));CHECK(!memcmp(a,v->pcm,na));if(nb)CHECK(!memcmp(b,v->pcm+na,nb));IDirectSoundBuffer_Unlock(message.buffer[0],a,na,b,nb);
  sniper_audio_voice_play(&p,&message,&req);IDirectSoundBuffer_GetStatus(message.buffer[0],&status);CHECK((status&5)==5);
  sniper_audio_play(&p,sniper_audio_assets[i].resource,&req);
  sniper_audio_set_active(&p,0);CHECK(!p.active&&p.enabled);
  IDirectSoundBuffer_GetStatus(v->buffer[0],&status);CHECK(!(status&1));
  /* Native cache stop excludes the independently owned message voice. */
  IDirectSoundBuffer_GetStatus(message.buffer[0],&status);CHECK((status&5)==5);
  IDirectSoundBuffer_Stop(message.buffer[0]);int inactive_next=v->next;
  sniper_audio_play(&p,sniper_audio_assets[i].resource,&req);CHECK(inactive_next==v->next);
  sniper_audio_voice_play(&p,&message,&req);IDirectSoundBuffer_GetStatus(message.buffer[0],&status);CHECK(!(status&1));
  sniper_audio_enable(&p,1);CHECK(!p.active);sniper_audio_voice_play(&p,&message,&req);IDirectSoundBuffer_GetStatus(message.buffer[0],&status);CHECK(!(status&1));
  sniper_audio_set_active(&p,1);CHECK(p.active);IDirectSoundBuffer_GetStatus(message.buffer[0],&status);CHECK(!(status&1));
  sniper_audio_voice_play(&p,&message,&req);IDirectSoundBuffer_GetStatus(message.buffer[0],&status);CHECK((status&5)==5);
  sniper_audio_voice_close(&message);CHECK(!message.buffer[0]&&!message.count);sniper_audio_voice_close(&message);
  sniper_audio_stop(&p,sniper_audio_assets[i].resource);IDirectSoundBuffer_GetStatus(v->buffer[0],&status);CHECK(!(status&1));
 }
 sniper_audio_close(&p);CHECK(!p.device);sniper_audio_close(&p);DestroyWindow(w);
 printf("PASS17 embedded PCM assets,%d DirectSound voices,buffer contents,loop/status,volume,pan,frequency,mute,stop,release and17 memory-message copies/activity-gate traces; null device (no audible-output claim)\n",voices);return 0;
}
