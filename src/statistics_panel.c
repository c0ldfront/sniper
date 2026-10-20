#include "statistics_panel.h"
#include <stdio.h>
#include <string.h>
static int32_t bits(uint32_t v){int32_t s;memcpy(&s,&v,4);return s;}
int32_t sniper_statistics_toggle(int32_t mode,int32_t *dirty){*dirty=1;return mode==2?0:mode==4?2:4;}
int sniper_statistics_update(SniperStatisticsPanel *p,SniperShipObject *const *ships,int32_t count,
    const SniperShipObject *local,int32_t history,int32_t *selected,int32_t *dirty,
    int compact,uint32_t (*clock_ms)(void *),void *user) {
    if(count<=0||count>1024||history<=0||history>256||*selected<0||p->scroll<0)return -1;
    *selected%=count;
    if((p->valid&&clock_ms(user)/10u-p->tick>300u)||*dirty){*dirty=0;p->valid=0;}
    int32_t visible=count<history?count:history;
    if(p->scroll>*selected)p->scroll=*selected;
    if(p->scroll>count-visible)p->scroll=count-visible;
    if(p->scroll+visible<=*selected)p->scroll=*selected-visible+1;
    if(p->valid)return 0;
    snprintf(p->title,sizeof p->title,compact?" ^ %3d ~    Points":" ^ %3d ~      Squad         W   L   R",(int)count);
    p->count=p->per=visible;
    for(int32_t i=0;i<visible;++i) {
        SniperShipObject *s=ships[p->scroll+i];const char *name=(const char *)s->word,*squad=name+32;
        int marker=p->scroll+i==*selected?'>':' ';
        int32_t wins=s->word[0x78/4],losses=s->word[0x7c/4],points=s->word[0x80/4];
        if(compact) {
            if(s->word[0x40/4]==4)snprintf(p->text[i],128,"%c%-11.11s (S)",marker,name);
            else snprintf(p->text[i],128,"%c%-11.11s%6d",marker,name,(int)points);
        } else {
            int32_t numerator=bits((((uint32_t)wins-(uint32_t)losses)*10u+(uint32_t)points)*10u);
            int32_t denominator=bits((uint32_t)wins+100u);
            if(!denominator)return -1;
            int32_t rating=(numerator>0?numerator:0)/denominator;
            if(s->word[0x40/4]==4)snprintf(p->text[i],128,"%c%-12.12s %-11.11s  (Spectator)",marker,name,squad);
            else snprintf(p->text[i],128,"%c%-12.12s %-11.11s %3d %3d %3d",marker,name,squad,(int)wins,(int)losses,(int)rating);
        }
        p->rows[i]=(SniperPanelRow){p->text[i],s->word[0x68/4]==local->word[0x68/4]?2:1};
    }
    p->tick=clock_ms(user)/10u;p->valid=1;return 1;
}
