#include "../platform/audio_message.h"
#include "../src/network_compression.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){printf("FAIL line%d\n",__LINE__);return 1;}}while(0)
int main(void){
 HINSTANCE instance=GetModuleHandleA(NULL);HWND w=CreateWindowExA(0,"STATIC","message-test",WS_OVERLAPPEDWINDOW,0,0,320,200,NULL,NULL,instance,NULL);CHECK(w);
 SniperAudioPlayer p;SniperAudioVoice voice={0};CHECK(SUCCEEDED(sniper_audio_open(&p,w,instance)));
 int compressed=0;
 for(int i=0;i<17;i++)for(int pass=0;pass<2;pass++){
  HRSRC r=FindResourceA(instance,MAKEINTRESOURCEA(sniper_audio_assets[i].resource),"WAVE");DWORD size=SizeofResource(instance,r);const uint8_t *raw=LockResource(LoadResource(instance,r));
  int32_t length;uint8_t *data=sniper_network_compress(raw,(int32_t)size,&length,0,(uint32_t)pass);CHECK(data);if(data[3])compressed++;
  sniper_audio_set_active(&p,pass==0);CHECK(SUCCEEDED(sniper_audio_message(&p,&voice,data,length)));memset(data,0,(size_t)length);free(data);
  CHECK(voice.count==1&&!voice.pcm);DWORD status;IDirectSoundBuffer_GetStatus(voice.buffer[0],&status);CHECK((status&1)==(pass==0));
  SniperAudioWave wave;CHECK(sniper_audio_wave(raw,size,&wave));CHECK(wave.bytes==voice.bytes);
  void *a,*b;DWORD na,nb;CHECK(SUCCEEDED(IDirectSoundBuffer_Lock(voice.buffer[0],0,voice.bytes,&a,&na,&b,&nb,0)));CHECK(!memcmp(a,wave.pcm,na));if(nb)CHECK(!memcmp(b,wave.pcm+na,nb));IDirectSoundBuffer_Unlock(voice.buffer[0],a,na,b,nb);
  IDirectSoundBuffer *previous=voice.buffer[0];CHECK(sniper_audio_message(&p,&voice,NULL,length)==S_FALSE&&voice.buffer[0]==previous);
 }
 uint8_t invalid[4]={0,0,0,1};CHECK(FAILED(sniper_audio_message(&p,&voice,invalid,4))&&!voice.count&&!voice.buffer[0]);
 sniper_audio_voice_close(&voice);sniper_audio_close(&p);DestroyWindow(w);
 printf("PASS34 cached-DATA message uploads,%d compressed,PCM survives input free,active/inactive playback,NULL no-op,malformed replacement and cleanup\n",compressed);return 0;
}
