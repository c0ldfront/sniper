#ifndef SNIPER_AUDIO_LOGIC_H
#define SNIPER_AUDIO_LOGIC_H
#include <stdint.h>
#include <stddef.h>
typedef struct {int32_t resource, voices; uint32_t original_global;} SniperAudioAsset;
extern const SniperAudioAsset sniper_audio_assets[17];
typedef struct {int32_t volume,pan,loop,pitch;} SniperAudioRequest;
/* 407a80. Caller obtains RNG only after geometry returns1. Units: listener
 * thousandths of pixels, source pixels. Invalid native divisors return0. */
int sniper_audio_position(int32_t x,int32_t y,int32_t listener_x,int32_t listener_y,
                          int listener_present,int32_t width,SniperAudioRequest *);
int32_t sniper_audio_pitch(int32_t random,int32_t spread);
int32_t sniper_audio_frequency(int32_t base,int32_t pitch);
int sniper_audio_should_play(int available,int enabled,int count,int status_ok,uint32_t status);
/* 40ad50: last fmt/data chunks win, scan ends at RIFF's declared size.
 * Format copies18 bytes like native,including bytes beyond a16-byte fmt chunk
 * when present in the supplied buffer. Malformed/out-of-bounds input fails. */
typedef struct {
    uint8_t format[18];
    const uint8_t *pcm;
    uint32_t bytes;
    int has_format;
} SniperAudioWave;
int sniper_audio_wave(const uint8_t *,size_t,SniperAudioWave *);
#endif
