#include "hud_status.h"
#include "font.h"
#include <stdio.h>
static int32_t add(int32_t a,int32_t b){return (int32_t)((uint32_t)a+(uint32_t)b);}
static int32_t sub(int32_t a,int32_t b){return (int32_t)((uint32_t)a-(uint32_t)b);}
static int32_t mul(int32_t a,int32_t b){return (int32_t)((uint32_t)a*(uint32_t)b);}
static void right_text(SniperHudStatus *h,const char *s,int32_t y,int color)
{
    h->text(h->user,s,sub(sub(h->width,sniper_font_width(s)),1),y,color);
}
void sniper_hud_status(SniperHudStatus *h)
{
    char text[128];
    int32_t energy=h->local->word[0x5c/4];
    if(energy>=0){
        snprintf(text,sizeof text,"%d",energy/1000);
        energy=h->local->word[0x5c/4];
        uint32_t color;
        if(energy>800000){right_text(h,text,16,3);color=h->energy_high;}
        else if(energy>500000){right_text(h,text,16,3);color=h->energy_medium;}
        else{right_text(h,text,16,2);color=h->energy_low;}
        if(h->show_energy_bar){
            int32_t span=h->width/3,left=sub(h->width,span)/2;
            energy=h->local->word[0x5c/4];
            int32_t right=add(left,mul(energy,span)/h->maximum_energy);
            int draw=1;
            if(energy<=250000){draw=h->blink%6==0;h->blink=add(h->blink,1);}
            if(draw)h->rect(h->user,left,1,right,5,color);
            h->vertical_line(h->user,sub(left,1),0,6,h->border);
            h->vertical_line(h->user,add(left,span),0,6,h->border);
            int32_t capacity=add(left,mul(h->local->word[0xc8/4],span)/h->maximum_energy);
            h->vertical_line(h->user,capacity,0,6,h->capacity);
        }
    }
    int32_t timer=h->local->word[0x108/4];
    if(timer>0){snprintf(text,sizeof text,"Shields: %d.%d",timer/100,(timer/10)%10);right_text(h,text,32,2);}
    timer=h->local->word[0x10c/4];
    if(timer>0){snprintf(text,sizeof text,"Super!: %d.%d",timer/100,(timer/10)%10);right_text(h,text,40,2);}
    timer=h->safety_ticks;
    if(timer>0){
        snprintf(text,sizeof text,"Safety Time Left: %d:%02d:%02d",timer/360000,(timer%360000)/6000,(timer%6000)/100);
        h->text(h->user,text,h->width/2,sub(h->height/2,48),2);
    }
    if(h->player_count>1&&h->connected){
        uint32_t now=h->milliseconds(h->user)/10;
        uint32_t last=h->last_receive_tick(h->user);
        if(now-last>1500u)h->text(h->user,"WARNING: No data coming from server",h->width/6,mul(h->height,2)/5,2);
    }
}
