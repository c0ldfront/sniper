#ifndef SNIPER_MOVEMENT_H
#define SNIPER_MOVEMENT_H
#include <stdint.h>
typedef struct { int32_t x, y, vx, vy; } SniperMotion;
typedef struct {
    int32_t player_id, death_ticks, respawn_ticks, attached_id;
    /* Original ship+0x48, which can differ from motion.x for prediction. */
    int32_t *ship_x;
} SniperMoveShip;
typedef struct { int32_t x, y, value; } SniperPickup;
typedef struct {
    uint8_t *map;
    int32_t local_id, watched_id, camera_x, camera_y, bounce_divisor;
    SniperPickup *pickups;
    int32_t pickup_count;
    /* Preserve synchronous side effects: callback may update map or state.
     * local_request selects network virtual +0x1c vs local routine 0x404910. */
    void (*pickup)(void *, int32_t x, int32_t y, int32_t player,
                   int32_t value, int local_request);
    void (*impact_sound)(void *, int32_t x_pixels, int32_t y_pixels);
    void *user;
} SniperMoveWorld;
/* Recovered 0x414450..0x414b0e. Caller supplies valid map and nonzero bounce
 * divisor; original also traps on invalid division. Coordinates are 1/1000 px.
 * effects is original final argument: enables pickups and impact sound. */
void sniper_move(SniperMotion *, SniperMoveShip *, SniperMoveWorld *,
                 int32_t ticks, int effects);
/* Same operation on original aliased object fields; callbacks see live position. */
void sniper_move_fields(int32_t *x,int32_t *y,int32_t *vx,int32_t *vy,
    SniperMoveShip *,SniperMoveWorld *,int32_t ticks,int effects);
/* Actual ship update path: read identity/timers/attachment live after pickups.
 * Object must contain original fields through offset0x6c. */
void sniper_move_object(int32_t *object,SniperMoveWorld *,int32_t ticks,int effects);
#endif
