#ifndef SNIPER_NETWORK_LOGIN_H
#define SNIPER_NETWORK_LOGIN_H
#include "network_game_packets.h"
typedef struct {
    uint8_t status;
    int32_t version;
    uint32_t map_checksum,exe_checksum,arena_checksum;
} SniperLoginReply;
/*98-byte09 packet. Reject names/passwords over31 bytes. Native uninitialized
 * tails are deliberately zeroed; reserved final32-byte field is empty. */
int sniper_login_packet(SniperGamePacket *,const char *nickname,const char *password,int create_user);
/*0 ignored,1 parsed,-1 malformed. status1=new-user question,2=password,
 *3=locked out; every other status follows native success path. */
int sniper_login_reply(SniperLoginReply *,const uint8_t *,size_t);
int sniper_login_accepted(const SniperLoginReply *);
int sniper_login_version_matches(const SniperLoginReply *,uint32_t local_exe_checksum);
void sniper_login_join_packet(SniperGamePacket *,int32_t ship,int32_t team,int32_t allow_audio,int32_t width,int32_t height);
void sniper_login_map_request(SniperGamePacket *);
void sniper_login_arena_request(SniperGamePacket *);
void sniper_login_cancel_download(SniperGamePacket *);
/* Native cache basename: host+port, remove '.', append '.lvl', retain last12.
 * Bounds-checked caller buffer of13. Host/port are trusted settings strings. */
int sniper_login_map_name(char output[13],const char *host,const char *port);
typedef struct { int32_t offset,total,done; } SniperLoginDownload;
/* Download packets0C offset+bytes,0D total. write callback borrows bytes.
 * Return1 recognized,0 ignored,-1 malformed; offset -1 marks completion. */
int sniper_login_download(SniperLoginDownload *,const uint8_t *,size_t,
    void (*write_at)(void *,int32_t,const uint8_t *,size_t),void *);
#endif
