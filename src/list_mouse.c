#include "list_mouse.h"
#include <string.h>
static int32_t add(int32_t a,uint32_t b){uint32_t u=(uint32_t)a+b;int32_t s;memcpy(&s,&u,4);return s;}
int sniper_mouse_repeat(int32_t *timer,int32_t ticks,int32_t left,int32_t right) {
    if(left||right)*timer=12;
    *timer=add(*timer,0u-(uint32_t)ticks);
    if(*timer<0){*timer=8;return 1;}return 0;
}
void sniper_list_mouse(SniperListMouse *s) {
    if(!s->left_held||!s->present){s->drag=0;return;}
    int32_t bottom=add(s->height,8),row_end=add(bottom,UINT32_MAX-1u);
    if(s->y>8&&s->y<23&&(s->repeat||s->left_event)) {
        if(s->x<21) {if(s->selected>0){s->dirty=1;s->selected=add(s->selected,UINT32_MAX);}}
        else if(s->x>39&&s->x<57&&add(s->count,UINT32_MAX)>s->selected){s->dirty=1;s->selected=add(s->selected,1);}
    }
    if(s->left_event&&s->x<=s->width) {
        if(s->y>row_end&&s->y<add(bottom,3))s->drag=1;
        else if(s->y>=23&&s->y<row_end&&s->count) {
            s->dirty=1;s->selected=add(add(s->y,0u-23u)/8,(uint32_t)s->scroll)%s->count;
        }
    }
    if(s->drag) {
        int32_t size=add(s->y,0u-22u)/8,limit=s->screen_height/24;
        if(size>=limit)size=limit;
        if(size<=1)size=1;
        if(size!=s->history)s->dirty=1;
        s->history=size;
    }
}
