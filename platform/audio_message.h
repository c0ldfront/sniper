#ifndef SNIPER_AUDIO_MESSAGE_H
#define SNIPER_AUDIO_MESSAGE_H
#include "audio_player.h"
/* 404090 audio half: complete cached DATA bytes,including compression pass
 * header; not a transport packet or naked WAV. Replaces caller-owned voice,
 * decodes,uploads then plays volume0/pan0/loop0/pitch0. No recipient/ReceiveWave
 * filter here; native caller handles routing and shows its notice beforehand.
 * NULL data is offline4115e0's no-op and returns S_FALSE. */
HRESULT sniper_audio_message(SniperAudioPlayer *,SniperAudioVoice *,const uint8_t *,int32_t);
#endif
