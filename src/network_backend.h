#ifndef SNIPER_NETWORK_BACKEND_H
#define SNIPER_NETWORK_BACKEND_H
#include "network_game_receive.h"
#include "player_registry.h"
#include "death_event.h"
#include "movement.h"
#include "attachment.h"
#define SNIPER_NETWORK_PLAYER_LIMIT 255
/* Borrowed live objects. Weapon callback receives409d70's14 args; prediction
 * Supply movement or predict, and weapon when packets can contain weapons.
 * override supports a host's414450 binding, NULL uses recovered movement. */
typedef struct {
 SniperShipObject **players;
 SniperMoveWorld *movement;
 uint32_t (*clock_ms)(void *);
 void (*weapon)(void *,const int32_t args[14]);
 void (*predict)(void *,SniperShipObject *,int32_t ticks,int32_t *x,int32_t *y,int32_t *vx,int32_t *vy);
 void *user;
} SniperNetworkPosition;
void sniper_network_position(SniperNetworkPosition *,const int32_t args[15]);
typedef struct {
 SniperPlayerRegistry *registry; /* canonical count/selected/list dirty/local */
 SniperPlayerTeam *team;
 SniperDeathWorld *death;
 SniperPickupWorld *pickups;
 SniperAttachmentHud *attachment_hud;
 SniperNetworkPosition *position;
 int32_t *local_id;
 char (*chat_lines)[128];int32_t *chat_colors;
 int32_t *chat_rows,*screen_width,*chat_dirty;
 SniperPickupObject *(*allocate_pickup)(void *);
 uint32_t (*clock_ms)(void *);
 /* File workflow/data events forwarded here; decoder alone never writes files.
  * Missing subsystem/event and invalid packet-world arguments also forwarded. */
 void (*extension)(void *,const SniperGameEvent *);
 void *user;
} SniperNetworkBackend;
void sniper_network_backend_event(void *,const SniperGameEvent *);
#endif
