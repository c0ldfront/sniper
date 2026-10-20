#include "audio_message.h"
#include "../src/network_compression.h"
#include <stdlib.h>
HRESULT sniper_audio_message(SniperAudioPlayer *player,SniperAudioVoice *voice,const uint8_t *data,int32_t size){
 if(!data)return S_FALSE;
 int32_t decoded_size=0;
 uint8_t *decoded=sniper_network_decompress(data,size,&decoded_size);
 /* Native replaces the previous voice after decode,even if the new payload
  * cannot construct a playable buffer. Invalid input is handled safely here. */
 sniper_audio_voice_close(voice);
 if(!decoded)return DSERR_BADFORMAT;
 HRESULT result=sniper_audio_memory_wave(player,voice,decoded,(size_t)decoded_size);
 free(decoded);
 if(SUCCEEDED(result)){SniperAudioRequest request={0};sniper_audio_voice_play(player,voice,&request);}
 return result;
}
