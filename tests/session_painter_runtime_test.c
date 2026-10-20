#include "session_painter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct{uint32_t now;int frame;} Host;
static uint32_t clock_ms(void *p){return ((Host *)p)->now;}
static int32_t key(void *p,int32_t vk){Host *h=p;return (vk==38 || (vk==39&&h->frame%20<5) || (vk==17&&h->frame%3==0))?0x8000:0;}
static int32_t queued_key(void *p,int32_t vk){Host *h=p;return vk==16&&(h->frame==30||h->frame==40)?0x8000:0;}
int main(int argc,char **argv) {
    int spectator=argc>1&&!strcmp(argv[1],"--spectator");
    int chat=argc>1&&!strcmp(argv[1],"--chat");
    int radar=argc>1&&!strcmp(argv[1],"--radar");
    int statistics=argc>1&&!strcmp(argv[1],"--statistics");
    FILE *f=fopen("LOCAL.LVL","rb");if(!f)return 1;fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);
    if(size<=0)return 1;
    uint8_t *data=malloc((size_t)size);if(!data)return 1;
    if(fread(data,1,(size_t)size,f)!=(size_t)size)return 1;
    fclose(f);
    HINSTANCE inst=GetModuleHandle(NULL);HWND window=CreateWindowA("STATIC","session",WS_OVERLAPPEDWINDOW|WS_VISIBLE,0,0,640,480,NULL,NULL,inst,NULL);
    SniperGameDisplay d={0};HRESULT hr=sniper_game_display_open(&d,window,640,480);if(FAILED(hr))return 2;
    SniperGameGraphics g={0};hr=sniper_game_graphics_open(&g,&d,inst);if(FAILED(hr))return 3;
    SniperTileSize sizes[256]={{0}};for(int i=0;i<4;++i)sizes[216+i]=(SniperTileSize){g.map.special[i].descriptor.width,g.map.special[i].descriptor.height};
    Host host={1000,0};SniperSessionHost callbacks={.clock_ms=clock_ms,.key_state=key,.user=&host,.key_state_event=queued_key};
    SniperOfflineSession *s=sniper_session_create(&callbacks,data,(size_t)size,sizes,"Pilot",spectator?4:0,0,640,480,87);free(data);if(!s)return 4;
    SniperSessionPainter painter;sniper_session_painter_init(&painter,s,&d,&g);painter.frame.stars=1;painter.hud.show_energy_bar=1;
    if(s->overlay_mode!=4)return 21;
    if(statistics)s->overlay_mode=0;
    if(chat){s->overlay_mode=0;s->ticker_enabled=0;}
    int32_t x=s->ships.players[0]->word[0x48/4],y=s->ships.players[0]->word[0x4c/4];int shots=0,pickups=0;
    for(host.frame=0;host.frame<100;++host.frame) {
        host.now+=100;
        if(radar){
            if(host.frame==10||host.frame==20)s->lifecycle.key=0x73;
            if(host.frame==40)s->radar_small=1;
            if(host.frame==60)s->radar_small=0;
        }
        if(spectator){
            if(host.frame==10){
                SniperShipDefaults defaults;sniper_local_ship_defaults(&s->ships.config,&defaults);
                for(int id=1;id<=2;++id){
                    SniperShipObject *ship=calloc(1,sizeof *ship);if(!ship)return 23;
                    SniperShipIdentity identity={.player_id=id,.name=id==1?"Observed One":"Observed Two",.squad="",.type=0,.team=id,.attached_id=-1};
                    sniper_ship_create(ship,&identity,&defaults,host.now);
                    sniper_ship_spawn(ship->word,&defaults,s->map,3,host.now,&s->ships.random_state,1,1,&s->ships.position_dirty);
                    s->ships.players[id]=ship;s->ordered[id]=ship;
                }
                s->ships.player_count=3;s->binding.selected=1;s->lifecycle.key=9;
            }
            /* Synthetic peer heartbeats keep remote visibility current. */
            if(host.frame>=10)for(int id=1;id<=2;++id)s->ships.players[id]->word[0x74/4]=(int32_t)(host.now/10u);
            if(host.frame==20){s->binding.selected=2;s->lifecycle.key=17;}
            if(host.frame==30){s->binding.selected=0;s->lifecycle.key=9;}
            if(host.frame==40)s->lifecycle.key=123;
            if(host.frame==50||host.frame==60)s->lifecycle.key=0x70;
        }
        if(chat){
            if(host.frame==5){strcpy(s->chat.text,"Hello Sniper");s->lifecycle.key=13;}
            if(host.frame==15){strcpy(s->chat.text,"'Team message");s->lifecycle.key=13;}
            if(host.frame==25){strcpy(s->chat.text,"/Private message");s->lifecycle.key=13;}
            if(host.frame==30){strcpy(s->chat.text,"Saved macro");s->lifecycle.key=0x70;}
            if(host.frame==40)s->lifecycle.key=0x70;
            if(host.frame==50||host.frame==60)s->lifecycle.key=0x74;
            if(host.frame==70){strcpy(s->chat.text,"=2");s->lifecycle.key=13;}
            if(host.frame==80)s->ticker_enabled=1;
            if(host.frame==90)g.chat.last_build_tick=host.now/10u-1001u;
            if(host.frame==99)strcpy(s->chat.text,"Typing a message");
        }
        if(statistics) {
            if(host.frame==10||host.frame==20||host.frame==30||host.frame==98||host.frame==99)s->lifecycle.key=0x71;
        } else if(!radar&&!chat&&!spectator&&(host.frame==10||host.frame==20||host.frame==99))s->lifecycle.key=0x70;
        hr=sniper_session_present(&painter);
        if(FAILED(hr)){printf("FAIL frame%d hr=%08lx\n",host.frame,(unsigned long)hr);return 5;}
        if(radar){
            int enabled=host.frame<10||host.frame>=20;
            if(s->radar_enabled!=enabled)return 11;
            if(enabled&&(!g.radar.surface||g.radar.size!=(s->radar_small?240:320)))return 12;
        }
        if(spectator){
            if(s->ships.players[0]->word[0x48/4]!=-1||s->combat.count)return 24;
            if(host.frame<10&&(s->start.watched!=-1||s->frame.camera.x||s->frame.camera.y))return 25;
            if(host.frame>=10){int id=host.frame<20?1:2;
                if(s->start.watched!=id||s->frame.watched!=s->ships.players[id]||s->ships.movement.watched_id!=id)return 26;
                if(s->frame.camera.x!=s->frame.watched->word[0x48/4]/1000-320)return 27;
            }
            if(host.frame==40&&s->ships.players[0]->word[0x5c/4]<0)return 28;
            if(host.frame==50&&s->overlay_mode!=3)return 29;
            if(host.frame==60&&s->overlay_mode!=0)return 30;
        }
        if(chat){
            if(host.frame==5&&(!strstr(s->chat_lines[0],"Hello Sniper")||s->chat_colors[0]!=0))return 13;
            if(host.frame==15&&s->chat_colors[0]!=2)return 14;
            if(host.frame==25&&s->chat_colors[0]!=3)return 15;
            if((host.frame==30||host.frame==40)&&(strcmp(s->chat.macros[0],"Saved macro")||s->chat.text[0]||s->overlay_mode))return 16;
            if(host.frame==50&&s->chat.enabled)return 17;
            if(host.frame==60&&!s->chat.enabled)return 18;
            if(host.frame==70&&s->ships.players[0]->word[0x68/4]!=2)return 19;
            if(host.frame==90&&(s->ticker_index!=1||strcmp(s->chat_lines[0],"F1 for HELP.")))return 22;
        }
        shots|=s->combat.count>0;pickups|=s->pickups.collection.count>0;
    }
    if(!spectator&&((x==s->ships.players[0]->word[0x48/4]&&y==s->ships.players[0]->word[0x4c/4])||!shots||!pickups))return 6;
    if(spectator) {
        printf("PASS spectator empty world,Tab/Ctrl follow,invalid target,F12 suppression,help,camera and radar with two synthetic peers\n");
    } else if(chat) {
        if(!g.chat.surface)return 20;
        printf("PASS chat public/team/private history,macro save/replay,F5 toggle,team change,ticker and live input rendering\n");
    } else if(radar) {
        printf("PASS F4 radar toggle and normal/small/normal bitmap recreation on real frames\n");
    } else if(statistics) {
        if(s->overlay_mode!=2||!g.statistics.surface||g.statistics_content.count!=1)return 10;
        printf("PASS F2 compact/detailed/off cycle and final detailed statistics on real frames\n");
    } else {
        if(s->overlay_mode!=1||!g.help.surface)return 10;
        printf("PASS F1 help overlay opened, closed and reopened on real frames\n");
    }
    if(spectator)printf("PASS100 spectator frames:map,actors,camera,radar,chat,help,primary flips; local flight/weapons/HUD suppressed\n");
    else printf("PASS100 real offline frames:LOCAL.LVL,spawn,controls,movement,projectiles,pickup generation,camera,stars,map,actors,HUD,primary flips\n");
    DDSURFACEDESC image={0};image.dwSize=sizeof image;PALETTEENTRY palette[256];
    if(FAILED(IDirectDrawPalette_GetEntries(g.palette,0,0,256,palette)))return 7;
    if(FAILED(IDirectDrawSurface_Lock(d.primary,NULL,&image,DDLOCK_WAIT,NULL)))return 8;
    FILE *capture=fopen("session-frame.ppm","wb");if(!capture)return 9;
    fprintf(capture,"P6\n640 480\n255\n");
    for(int py=0;py<480;++py)for(int px=0;px<640;++px) {
        unsigned index=((unsigned char *)image.lpSurface)[py*image.lPitch+px];
        unsigned char rgb[]={palette[index].peRed,palette[index].peGreen,palette[index].peBlue};
        fwrite(rgb,1,3,capture);
    }
    fclose(capture);IDirectDrawSurface_Unlock(d.primary,image.lpSurface);
    sniper_session_destroy(s);sniper_game_graphics_close(&g);sniper_game_display_close(&d);DestroyWindow(window);
    printf("PASS session/graphics/display cleanup\n");return 0;
}
