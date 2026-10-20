#ifndef SNIPER_NETWORK_GAME_PACKETS_H
#define SNIPER_NETWORK_GAME_PACKETS_H
#include <stddef.h>
#include <stdint.h>
/* Recovered online backend wire encoders. These do not apply local gameplay
 * callbacks or perform I/O. reliable/copy are 401610 arguments, not wire bytes. */
typedef struct { uint8_t data[512]; uint32_t length, reliable, copy; } SniperGamePacket;
void sniper_packet_position(SniperGamePacket *, const int32_t args[13], uint32_t milliseconds, uint32_t clock_offset);
void sniper_packet_death(SniperGamePacket *, int32_t kind, int32_t player);
void sniper_packet_pickup(SniperGamePacket *, int32_t x,int32_t y,int32_t prize,int32_t player,uint32_t reliable);
int sniper_packet_chat(SniperGamePacket *,int32_t recipient,const char *text,uint32_t broadcast_reliable);
void sniper_packet_ship(SniperGamePacket *,int32_t ship);
void sniper_packet_team(SniperGamePacket *,int32_t team);
void sniper_packet_attach(SniperGamePacket *,int32_t player);
void sniper_packet_release_children(SniperGamePacket packets[2],int32_t team);
/* Null payload is the original no-op. cached is per backend slot, updated on
 * first send; caller owns original local 404090 callback after a send. */
size_t sniper_packet_cached_into(uint8_t *output,size_t capacity,uint32_t *copy,int32_t recipient,int32_t slot,const uint8_t *payload,size_t length,int *cached);
int sniper_packet_cached(SniperGamePacket *,int32_t recipient,int32_t slot,const uint8_t *payload,size_t length,int *cached);
#endif
