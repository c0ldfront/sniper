#ifndef SNIPER_AUDIO_PLAYER_H
#define SNIPER_AUDIO_PLAYER_H
#define COBJMACROS
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include "../src/audio_logic.h"
typedef struct {IDirectSoundBuffer *buffer[16];int32_t count,next,frequency;const unsigned char *pcm;DWORD bytes;} SniperAudioVoice;
typedef struct {IDirectSound *device;SniperAudioVoice sounds[17];int enabled,active;} SniperAudioPlayer;
HRESULT sniper_audio_open(SniperAudioPlayer *,HWND,HINSTANCE);
/* Native40a840 activity gate; stops cached voices on false. Independent
 * memory voices already playing are not in that cache; future plays are gated. */
void sniper_audio_set_active(SniperAudioPlayer *,int);
void sniper_audio_enable(SniperAudioPlayer *,int);
void sniper_audio_close(SniperAudioPlayer *);
void sniper_audio_play(SniperAudioPlayer *,int resource,const SniperAudioRequest *);
void sniper_audio_stop(SniperAudioPlayer *,int resource);
void sniper_audio_stop_all(SniperAudioPlayer *);
/* Independent memory-message voice,not registered in resource stop-all cache.
 * Release before device close. Creation requires empty/uninitialized output;
 * input bytes can be released immediately after memory_wave returns. */
HRESULT sniper_audio_memory_wave(SniperAudioPlayer *,SniperAudioVoice *,const uint8_t *,size_t);
/* Resource variant retains borrowed bytes for restoration; count clamped1..16. */
HRESULT sniper_audio_voice_load(SniperAudioPlayer *,SniperAudioVoice *,const uint8_t *,size_t,int);
void sniper_audio_voice_play(SniperAudioPlayer *,SniperAudioVoice *,const SniperAudioRequest *);
void sniper_audio_voice_close(SniperAudioVoice *);
#endif
