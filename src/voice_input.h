#ifndef SNIPER_VOICE_INPUT_H
#define SNIPER_VOICE_INPUT_H
#include "notices.h"
typedef struct {
    SniperShipObject **ordered;
    int32_t count,selected,enabled,transport_present,large_transfer_count;
    const void *recordings[2];int32_t lengths[2];
    SniperNotices *notices;
    void (*send)(void *,int32_t recipient,int32_t slot,const void *,int32_t length);
    void *user;
} SniperVoiceInput;
/*405360..405506, reached only with queued Shift high bit. Returns1 when key
 * and repeat must be consumed, including all rejected ShiftF11/F12 cases.
 * Caller passes transport+bb4 (large transfer queue), not reliable queue count.
 * Negative selection safely ignored rather than native invalid pointer read. */
int sniper_voice_input(SniperVoiceInput *,int32_t key,int32_t shift);
#endif
