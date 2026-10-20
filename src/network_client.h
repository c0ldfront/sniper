#ifndef SNIPER_NETWORK_CLIENT_H
#define SNIPER_NETWORK_CLIENT_H
#include "network_transport.h"
#include "network_game_receive.h"
#include "network_login.h"
typedef struct {
    SniperTransportHost transport;
    void *user;
    void (*event)(void *,const SniperGameEvent *);
    void (*login_reply)(void *,const SniperLoginReply *);
    void (*download_write)(void *,int32_t,const uint8_t *,size_t);
    void (*download_done)(void *,const SniperLoginDownload *);
    void *event_user; /* NULL selects user; allows live-world binding without moving socket/download owner. */
} SniperNetworkClientHost;
typedef struct {
    SniperNetworkTransport transport;
    SniperGameReceive game;
    SniperNetworkClientHost host;
    SniperLoginReply login;
    SniperLoginDownload download;
    int login_pending,login_valid,download_active,join_sent;
    int32_t cached[16];
    uint32_t login_started;
} SniperNetworkClient;
/* Connection creation belongs network_socket. Init requires connected host. */
void sniper_network_client_init(SniperNetworkClient *,const SniperNetworkClientHost *);
void sniper_network_client_clear(SniperNetworkClient *);
int sniper_network_client_login(SniperNetworkClient *,const char *,const char *,int create_user);
int sniper_network_client_join(SniperNetworkClient *,int32_t ship,int32_t parameter,int32_t audio,int32_t width,int32_t height);
int sniper_network_client_request_map(SniperNetworkClient *);
int sniper_network_client_request_arenas(SniperNetworkClient *);
int sniper_network_client_cancel_download(SniperNetworkClient *);
/* Drains available packets. -1 malformed,-2 disconnected,0 no ready game,
 *1 game ready.
 * Login response/create-user decisions and checksum policy remain host-owned. */
int sniper_network_client_poll(SniperNetworkClient *);
int sniper_network_client_send(SniperNetworkClient *,const SniperGamePacket *);
void sniper_network_client_position(SniperNetworkClient *,const int32_t args[13]);
void sniper_network_client_death(SniperNetworkClient *,int32_t,int32_t);
int sniper_network_client_chat(SniperNetworkClient *,int32_t,const char *,uint32_t);
void sniper_network_client_pickup(SniperNetworkClient *,int32_t,int32_t,int32_t,uint32_t);
void sniper_network_client_ship(SniperNetworkClient *,int32_t);
void sniper_network_client_team(SniperNetworkClient *,int32_t);
void sniper_network_client_attach(SniperNetworkClient *,int32_t);
void sniper_network_client_release_children(SniperNetworkClient *,int32_t);
int sniper_network_client_cached(SniperNetworkClient *,int32_t recipient,int32_t slot,const uint8_t *,size_t);
#endif
