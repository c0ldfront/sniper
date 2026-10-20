#ifndef SNIPER_LOCAL_CONTROL_H
#define SNIPER_LOCAL_CONTROL_H
#include "ship_create.h"
typedef struct {
    SniperShipObject *local;
    int32_t key,repeat,mouse_x_event,mouse_y_event,joystick_enabled,local_id;
    uint8_t character;
    uint32_t joystick[13]; /* JOYINFOEX: size,flags,x,y,z,r,u,v,buttons,number,pov,reserved1/2 */
    void (*damage)(void *,SniperShipObject *,int32_t amount,int32_t owner);
    uint32_t (*poll_joystick)(void *,uint32_t id,uint32_t info[13]);
    /* In order:0 flight,1 weapons,2 position dispatch. Shared state/callback
     * context carries actual weapon output into position dispatch. */
    void (*stage)(void *,int stage);
    void *user;
} SniperLocalControl;
/* Ordinary local-control tail405f75..406aa6. Spectator/menu/chat handling
 * precedes this slice. Return0 for initially dead ship,1 after stages. */
int sniper_local_control(SniperLocalControl *);
#endif
