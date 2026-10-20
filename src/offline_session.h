#ifndef SNIPER_OFFLINE_SESSION_H
#define SNIPER_OFFLINE_SESSION_H
#include "offline_init.h"
#include "offline_start.h"
#include "offline_frame.h"
#include "starfield.h"
#include "list_mouse.h"
#include "chat_input.h"
#include "audio_logic.h"
#include "editor_input.h"
#include "editor_records.h"
#include "voice_recordings.h"
typedef struct {
    uint32_t (*clock_ms)(void *);
    int32_t (*key_state)(void *,int32_t);
    uint32_t (*joystick)(void *,uint32_t,uint32_t info[13]);
    void (*post_command)(void *);
    void (*cursor)(void *,int32_t,int32_t);
    void (*sound)(void *,int32_t,int32_t,int32_t);
    void *user;
    /* Queued GetKeyState semantics for command modifiers. When absent, uses
     * key_state; hosts with distinct queued/async state should supply this. */
    int32_t (*key_state_event)(void *,int32_t);
    void (*audio)(void *,int32_t,const SniperAudioRequest *);
    void (*audio_stop)(void *,int32_t);
    /* Optional persistent application clock. With NULL, standalone clients
     * retain bind-local initialization using their host clock. */
    SniperFrameClock *process_clock;
    /* Optional native CRT stream shared across sessions; overrides seed. */
    uint32_t *process_random_state;
} SniperSessionHost;
struct SniperOnlineSession;
typedef struct {
    struct SniperOnlineSession *online;
    SniperSessionHost host;
    SniperVoiceRecordings voice_recordings;
    void (*voice_receive)(void *,int32_t sender,const uint8_t *,size_t);
    void *voice_user;
    SniperShipWorld ships;
    SniperShipObject *ordered[1024];
    uint8_t map[SNIPER_MAP_BYTES];
    SniperStarfield stars;
    SniperOfflineCombat combat;
    SniperOfflinePickups pickups;
    SniperPickupObject *pickup_items[16];
    SniperPickup pickup_view[16];
    SniperLocalControl lifecycle;
    SniperPositionDispatch position;
    SniperOrdinaryControl control;
    SniperFrameUpdate frame;
    SniperOfflineFrame binding;
    SniperOfflineStart start;
    SniperChatInput chat;
    char chat_lines[16][128];
    int32_t chat_colors[16],chat_rows,ticker_enabled,ticker_index;
    int32_t overlay_mode;
    int32_t radar_enabled,radar_dirty,radar_small;
    int32_t label_mode,stars_enabled;
    SniperEditor editor;
    SniperEditorRecords editor_records;
    SniperTileSize editor_sizes[256];
    int editor_error,active;
    SniperListMouse mouse;
    int32_t mouse_repeat_ticks;
} SniperOfflineSession;
/* Owns map,ships,projectiles,pickups and persistent frame state. Caller owns
 * level bytes only during create;session copies their map meaning. Clock/key
 * callbacks required;other host callbacks optional (absent audio is muted).
 * This currently supplies ordinary offline controls,not command/spectator UI.
 * Sprite sizes must describe every large tile in the level. */
SniperOfflineSession *sniper_session_create(const SniperSessionHost *,const uint8_t *,size_t,
    const SniperTileSize sizes[256],const char *name,int32_t type,int32_t team,
    int32_t width,int32_t height,uint32_t seed);
SniperOfflineSession *sniper_session_create_editor(const SniperSessionHost *,const uint8_t *,size_t,
    const SniperTileSize sizes[256],int32_t width,int32_t height,uint32_t seed,
    int32_t cursor_x,int32_t cursor_y,int32_t selected_tile);
/* Common allocation has no synthetic local player; online handshake assigns it. */
SniperOfflineSession *sniper_session_prepare(const SniperSessionHost *,int32_t,int32_t,uint32_t);
void sniper_session_bind_local(SniperOfflineSession *,SniperShipObject *,int32_t);
SniperShipObject *sniper_session_local(SniperOfflineSession *);
void sniper_session_ticker(SniperOfflineSession *);
void sniper_session_destroy(SniperOfflineSession *);
/* Positional audio consumes original RNG even without a host audio callback.
 * host.sound is an optional legacy event observer. */
int sniper_session_step(SniperOfflineSession *);
#endif
