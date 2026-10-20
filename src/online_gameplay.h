#ifndef SNIPER_ONLINE_GAMEPLAY_H
#define SNIPER_ONLINE_GAMEPLAY_H
#include "offline_combat.h"
#include "offline_pickups.h"
#include "ordinary_control.h"
#include "network_client.h"
typedef struct {
 SniperOfflineCombat *combat;
 SniperOfflinePickups *pickups;
 SniperNetworkClient *client;
 int32_t local_id;
} SniperOnlineGameplay;
/* Existing storage and algorithms, arbitrary validated native ID0..254.
 * Client's event callback must apply its synchronous pickup/attach local echoes.
 * No independent pickup generation is performed by online simulation. */
void sniper_online_gameplay_damage(SniperOnlineGameplay *,SniperShipObject *,int32_t,int32_t);
void sniper_online_gameplay_construct(SniperOnlineGameplay *,SniperProjectile *,const int32_t[15]);
void sniper_online_gameplay_weapon(SniperOnlineGameplay *,const int32_t[14]);
void sniper_online_gameplay_combat_frame(SniperOnlineGameplay *,const int32_t *);
int sniper_online_gameplay_action(SniperOnlineGameplay *,int,int32_t,int);
int sniper_online_gameplay_input(SniperOnlineGameplay *,const SniperWeaponInput *,int32_t);
int sniper_online_gameplay_control(SniperOnlineGameplay *,SniperOrdinaryControl *);
void sniper_online_gameplay_simulation(SniperOnlineGameplay *,const int32_t *);
/* Bind once; preserves original movement sound context in pickups storage. */
void sniper_online_gameplay_sync_pickups(SniperOnlineGameplay *);
void sniper_online_gameplay_pickups_bind(SniperOnlineGameplay *);
void sniper_online_gameplay_collect(SniperOnlineGameplay *,int32_t,int32_t,int32_t,int32_t);
#endif
