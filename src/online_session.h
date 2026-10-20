#ifndef SNIPER_ONLINE_SESSION_H
#define SNIPER_ONLINE_SESSION_H
#include "offline_session.h"
#include "network_backend.h"
#include "online_gameplay.h"
typedef struct SniperOnlineSession {
    SniperOfflineSession *session;
    SniperNetworkClient *client;
    SniperPlayerRegistry registry;
    SniperShipDefaults defaults;
    SniperPlayerTeam team;
    SniperDeathWorld death;
    SniperNetworkPosition position;
    SniperNetworkBackend backend;
    SniperOnlineGameplay gameplay;
    void (*previous_event)(void *,const SniperGameEvent *);
    void *previous_event_user;
    int (*accept_event)(void *,const SniperGameEvent *);
    void *accept_event_user;
    int ready,failed;
} SniperOnlineSession;
/* Borrows connected client. Begins native join; caller polls until ready before
 * rendering/ordinary step. No synthetic/offline player is constructed. */
SniperOfflineSession *sniper_online_session_begin(const SniperSessionHost *,SniperNetworkClient *,
    const uint8_t *,size_t,const SniperTileSize[256],int32_t ship,int32_t team,int32_t audio,
    int32_t width,int32_t height,uint32_t seed);
int sniper_online_session_poll(SniperOfflineSession *);
/* Restores client event owner; socket/client lifetime remains connection-owned. */
void sniper_online_session_release(SniperOfflineSession *);
#endif
