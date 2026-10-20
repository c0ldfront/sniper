#ifndef SNIPER_LIST_MOUSE_H
#define SNIPER_LIST_MOUSE_H
#include <stdint.h>
typedef struct {
    int32_t x,y,left_held,left_event,repeat,drag,present,width,height;
    int32_t scroll,count,selected,history,dirty,screen_height;
} SniperListMouse;
int sniper_mouse_repeat(int32_t *timer,int32_t ticks,int32_t left_event,int32_t right_event);
/*405d56..405ec9. Header arrows repeat;rows select on a left-button event;
 * bottom border starts a history-size drag. A missing panel or released left
 * button clears drag. count0 row selection safely skips native divide fault. */
void sniper_list_mouse(SniperListMouse *);
#endif
