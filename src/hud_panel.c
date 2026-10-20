#include "hud_panel.h"
#include "font.h"
#include <stdio.h>
int sniper_team_panel_draw(SniperTeamPanel *p)
{
    uint32_t sum=(uint32_t)p->local->word[0x68/4]+(uint32_t)p->local->word[0xa0/4];
    SniperPanelOps *o=&p->ops;
    if(!p->surface||sum!=p->cached_sum){
        p->cached_sum=sum;
        if(p->surface){o->release(o->user,p->surface);p->surface=0;}
        p->surface=o->create(o->user,48,16);
        if(!p->surface)return -1;
        o->color_key(o->user,p->surface,p->background);
        o->clear(o->user,p->surface,p->background);
        char text[64];
        snprintf(text,sizeof text,"%d Ghz",p->local->word[0x68/4]);
        o->text(o->user,p->surface,text,48-sniper_font_width(text),0,1);
        snprintf(text,sizeof text,"%d Bty",p->local->word[0xa0/4]);
        o->text(o->user,p->surface,text,48-sniper_font_width(text),8,1);
    }
    o->blit(o->user,p->surface,(int32_t)((uint32_t)p->screen_width-48u),0,48,16);
    return 0;
}
int sniper_equipment_panel_draw(SniperEquipmentPanel *equipment)
{
    SniperTeamPanel *p=&equipment->cache;SniperPanelOps *o=&p->ops;
    static const unsigned offsets[]={0xf0,0xf4,0xd4,0x90,0xf8,0xd8,0x94,0xfc,0xec,0x98,0xdc,0x88,0x100,0xd0,0x8c};
    uint32_t sum=0;
    for(unsigned i=0;i<sizeof offsets/sizeof *offsets;++i)sum+=(uint32_t)p->local->word[offsets[i]/4];
    if(!p->surface||sum!=p->cached_sum){
        p->cached_sum=sum;
        if(p->surface){o->release(o->user,p->surface);p->surface=0;}
        p->surface=o->create(o->user,60,64);if(!p->surface)return -1;
        o->color_key(o->user,p->surface,p->background);
        o->clear(o->user,p->surface,p->background);
        char text[64];int32_t level=p->local->word[0xd0/4];
        if(level>0){snprintf(text,sizeof text,"Gun-%d",level);o->text(o->user,p->surface,text,0,0,1);}
        level=p->local->word[0xd4/4];
        if(level>0){
            snprintf(text,sizeof text,p->local->word[0x100/4]?"PBomb-%d:%d":"Bomb-%d:%d",level,p->local->word[0xd8/4]);
            o->text(o->user,p->surface,text,0,8,1);
        }
        static const char *labels[]={"Stealth","Cloak","Ghost","X-Radar","MultiFire"};
        for(int i=0;i<5;++i){
            if(!p->local->word[0xec/4+i])continue;
            int32_t y=16+i*8;
            if(p->local->word[0x88/4+i])equipment->rect(o->user,p->surface,0,y,sniper_font_width(labels[i]),y+8,equipment->highlight);
            equipment->transparent_text(o->user,p->surface,labels[i],0,y,0);
        }
        level=p->local->word[0xdc/4];
        if(level>0){snprintf(text,sizeof text,"Repel:%d",level);o->text(o->user,p->surface,text,0,56,0);}
    }
    o->blit(o->user,p->surface,0,equipment->screen_height/2,60,64);
    return 0;
}
