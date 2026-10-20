#ifndef SNIPER_NETWORK_GAME_RECEIVE_H
#define SNIPER_NETWORK_GAME_RECEIVE_H
#include <stddef.h>
#include <stdint.h>
/* Event kinds identify the native callee; FILE events represent the original
 * fopen/fwrite/fclose sequence. Pointer arguments occupy zero scalar slots and
 * are supplied in data/data2. Strings and payloads are borrowed for the callback. */
enum { SNIPER_NET_JOIN=0x404340, SNIPER_NET_LEAVE=0x404600,
 SNIPER_NET_POSITION=0x404a60,SNIPER_NET_DEATH=0x404680,
 SNIPER_NET_CHAT=0x404c50,SNIPER_NET_PICKUP=0x404910,
 SNIPER_NET_PICKUP_SPAWN=0x404f10,SNIPER_NET_DATA=0x404090,
 SNIPER_NET_TEAM=0x404530,SNIPER_NET_ATTACH=0x404fe0,
 SNIPER_NET_FILE_OPEN=0x4165c0,SNIPER_NET_FILE_WRITE=0x417c00,
 SNIPER_NET_FILE_CLOSE=0x4163d0 };
typedef struct { uint32_t kind,count; int32_t args[15];
 const uint8_t *data,*data2; size_t size,size2; } SniperGameEvent;
typedef struct { int32_t player_id,ready; uint32_t clock_offset;
 uint8_t settings[289]; } SniperGameReceive;
typedef struct {
 void *user;
 uint32_t (*clock_ms)(void *user);
 void (*event)(void *user,const SniperGameEvent *event);
 const uint8_t *(*next)(void *user,size_t *length);
} SniperGameReceiveHost;
/* Returns0 for malformed packet,1 consumed (including unknown/ignored opcode).
 * Host events apply the actual world mutation / file I/O; no I/O occurs here. */
int sniper_game_receive_packet(SniperGameReceive *,const SniperGameReceiveHost *,const uint8_t *,size_t);
void sniper_game_receive_drain(SniperGameReceive *,const SniperGameReceiveHost *);
#endif
