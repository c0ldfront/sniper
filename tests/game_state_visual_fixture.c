/* Controlled recreation render using read-only original ship/config/tick/pickup snapshot.
 * No simulation step, original executable unchanged, existing original map. */
#include "session_painter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t now(void *p){(void)p;return 1000;}
static int32_t key(void *p,int32_t v){(void)p;(void)v;return 0;}
int main(int argc,char **argv){
 int width=argc>1?atoi(argv[1]):640,height=argc>2?atoi(argv[2]):480;if(width<=0||height<=0)return 12;
 FILE *f=fopen("LOCAL.LVL","rb");if(!f)return 1;fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);uint8_t *data=malloc(size);if(fread(data,1,size,f)!=(size_t)size)return 2;fclose(f);
 HINSTANCE inst=GetModuleHandle(NULL);HWND window=CreateWindowA("STATIC","parity",WS_OVERLAPPEDWINDOW|WS_VISIBLE,0,0,width,height,NULL,NULL,inst,NULL);
 SniperGameDisplay d={0};if(FAILED(sniper_game_display_open(&d,window,width,height)))return 3;
 SniperGameGraphics g={0};if(FAILED(sniper_game_graphics_open(&g,&d,inst)))return 4;
 SniperTileSize sizes[256]={{0}};for(int i=0;i<4;++i)sizes[216+i]=(SniperTileSize){g.map.special[i].descriptor.width,g.map.special[i].descriptor.height};
 SniperSessionHost host={.clock_ms=now,.key_state=key};SniperOfflineSession *s=sniper_session_create(&host,data,size,sizes,"",0,0,width,height,87);free(data);if(!s)return 5;
 SniperShipObject *ship=sniper_session_local(s);f=fopen("/work/original-ship.bin","rb");if(!f)return 6;if(fread(ship->word,1,0x218,f)!=0x218||fread(&s->ships.config,1,288,f)!=288)return 7;uint32_t tick=0;fread(&tick,1,4,f);uint32_t count=0;fread(&count,1,4,f);if(count>16)return 10;
 for(uint32_t i=0;i<count;++i){SniperPickupObject *pickup=malloc(sizeof *pickup);if(!pickup||fread(pickup,1,20,f)!=20)return 11;s->pickup_items[i]=pickup;s->map[pickup->y*1024+pickup->x]=255;}
 s->pickups.collection.count=(int32_t)count;fclose(f);s->frame.clock.tick=tick;
 s->frame.camera.x=ship->word[0x48/4]/1000-width/2;s->frame.camera.y=ship->word[0x4c/4]/1000-height/2;
 s->ticker_enabled=0;s->overlay_mode=4;s->radar_enabled=1;s->chat.enabled=1;s->chat.dirty=1;
 strcpy(s->chat_lines[0],"F1 for HELP.");s->chat_colors[0]=3;
 SniperSessionPainter p;sniper_session_painter_init(&p,s,&d,&g);p.scene.tick=tick;p.frame.mode=4;p.frame.radar=1;p.frame.chat=1;p.frame.stars=0;p.order.count=1;p.hud.show_energy_bar=1;p.hud.player_count=1;p.hud.maximum_energy=s->ships.config.word[(0x428239-0x4281c9)/4];
 DDBLTFX fill={0};fill.dwSize=sizeof fill;
 for(int i=0;i<8;++i){if(FAILED(IDirectDrawSurface_Blt(d.backbuffer,NULL,NULL,NULL,DDBLT_COLORFILL|DDBLT_WAIT,&fill)))return 8;if(FAILED(sniper_scene_paint(&p.scene)))return 9;Sleep(50);}
 printf("PASS fixed render x=%ld y=%ld camera=%ld,%ld\n",(long)ship->word[0x48/4],(long)ship->word[0x4c/4],(long)s->frame.camera.x,(long)s->frame.camera.y);fflush(stdout);Sleep(15000);return 0;
}
