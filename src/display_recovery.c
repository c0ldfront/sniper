#include "display_recovery.h"
const uint32_t sniper_display_color_rgb[16]={
    0,0x404040,0x006000,0xffffff,0x7f0000,0xff0000,0xe18989,0x844848,
    0x0000ff,0x00007f,0x00ff00,0x32c6fd,0x808080,0x5c5c5c,0x303030,0x003000
};
int sniper_display_recover(SniperDisplayRecovery *s)
{
    if(s->restore(s->user,s->primary))return 0;
    if(s->restore(s->user,s->backbuffer))return 0;
    s->dirty[0]=s->dirty[1]=s->dirty[2]=1;
    if(s->team_panel){s->release(s->user,s->team_panel);s->team_panel=0;}
    if(s->equipment_panel){s->release(s->user,s->equipment_panel);s->equipment_panel=0;}
    s->set_palette(s->user,s->primary,s->palette);
    s->restore_tiles(s->user,s->tile_object);
    for(int i=0;i<16;++i)s->colors[i]=s->resolve_color(s->user,s->tile_surface,sniper_display_color_rgb[i]);
    return 1;
}
