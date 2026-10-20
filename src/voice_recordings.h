#ifndef SNIPER_VOICE_RECORDINGS_H
#define SNIPER_VOICE_RECORDINGS_H
#include <stdint.h>
typedef struct {uint8_t *data[2];int32_t length[2];} SniperVoiceRecordings;
/* Zero-initialize before first use.4034b5: snapshot complete record0.wav and
 * record1.wav from directory (NULL=>current directory),compress bypass0/pass2.
 * Missing/unreadable files leave NULL/0 slot. Call once per game entry; native
 * macro-dialog changes do not reload active-session snapshots. */
void sniper_voice_recordings_load(SniperVoiceRecordings *,const char *directory);
void sniper_voice_recordings_close(SniperVoiceRecordings *);
#endif
