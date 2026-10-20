#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "offline_session.h"
#include "ship_assets.h"
#include "ship_render.h"
#include "map_render.h"
#include "sniper_assets.h"

enum { VK_LEFT=37, VK_UP=38, VK_RIGHT=39, VK_DOWN=40, VK_SHIFT=16, VK_ESCAPE=27 };
typedef struct { uint32_t keys; int32_t event_key; } NativeInput;
typedef struct { SDL_Renderer *renderer; SDL_Texture *ships[9]; SDL_Texture *tiles[4]; } NativeGraphics;

static uint32_t clock_ms(void *u) { (void)u; return SDL_GetTicks(); }
static int32_t key_state(void *u, int32_t key) {
    NativeInput *in=u;
    if (key==in->event_key) return 0x8000;
    if (key==VK_LEFT) return (in->keys&1)?0x8000:0;
    if (key==VK_UP) return (in->keys&2)?0x8000:0;
    if (key==VK_RIGHT) return (in->keys&4)?0x8000:0;
    if (key==VK_DOWN) return (in->keys&8)?0x8000:0;
    if (key==VK_SHIFT) return (in->keys&16)?0x8000:0;
    return 0;
}
static uint32_t joystick(void *u, uint32_t id, uint32_t info[13]) { (void)u; (void)id; memset(info,0,13*sizeof(*info)); return 165; }
static int32_t virtual_key(SDL_Keycode key) {
    if(key==SDLK_LEFT)return VK_LEFT;
    if(key==SDLK_UP)return VK_UP;
    if(key==SDLK_RIGHT)return VK_RIGHT;
    if(key==SDLK_DOWN)return VK_DOWN;
    if(key==SDLK_LSHIFT||key==SDLK_RSHIFT)return VK_SHIFT;
    if(key==SDLK_ESCAPE)return VK_ESCAPE;
    if(key==SDLK_SPACE)return 32;
    if(key==SDLK_LCTRL||key==SDLK_RCTRL)return 17;
    if(key==SDLK_RETURN)return 13;
    if(key>=SDLK_F1&&key<=SDLK_F12)return 0x70+(int32_t)(key-SDLK_F1);
    if(key>=SDLK_a&&key<=SDLK_z)return (int32_t)('A'+key-SDLK_a);
    return 0;
}
static void post_command(void *u) { (void)u; }
static void cursor(void *u, int32_t x, int32_t y) { (void)u; (void)x; (void)y; }
static void load_level(const char *path, uint8_t **data, size_t *bytes) {
    FILE *f=fopen(path,"rb"); if(!f){fprintf(stderr,"cannot open level %s\n",path);exit(2);}
    if(fseek(f,0,SEEK_END)||ftell(f)<0){fclose(f);exit(2);} long n=ftell(f); rewind(f);
    *data=malloc((size_t)n); if(!*data||fread(*data,1,(size_t)n,f)!=(size_t)n){fclose(f);exit(2);} fclose(f); *bytes=(size_t)n;
}
/* RT_BITMAP resources are DIBs without the 14-byte BITMAPFILEHEADER; add it in memory. */
static SDL_Texture *load_texture(SDL_Renderer *r, unsigned id) {
    const SniperAsset *a=sniper_asset_find(2,NULL,id);
    if(!a||a->size<16){fprintf(stderr,"bitmap resource %u missing from generated assets\n",id);return NULL;}
    const unsigned char *d=a->data; uint32_t header=d[0]|d[1]<<8|(uint32_t)d[2]<<16|(uint32_t)d[3]<<24;
    uint32_t bits=header==12?(uint32_t)(d[10]|d[11]<<8):(uint32_t)(d[14]|d[15]<<8), used=0, compression=0;
    if(header>=40){used=d[32]|d[33]<<8|(uint32_t)d[34]<<16|(uint32_t)d[35]<<24;compression=d[16]|d[17]<<8;}
    uint32_t colors=used?used:bits<=8?1u<<bits:0, offset=14+header+colors*(header==12?3:4)+(header==40&&compression==3?12:0);
    size_t size=14+a->size; unsigned char *file=malloc(size); if(!file)return NULL;
    file[0]='B';file[1]='M';for(int i=0;i<4;i++){file[2+i]=(unsigned char)(size>>(8*i));file[10+i]=(unsigned char)(offset>>(8*i));}
    memset(file+6,0,4); memcpy(file+14,a->data,a->size);
    SDL_Surface *s=SDL_LoadBMP_RW(SDL_RWFromConstMem(file,(int)size),1); free(file);
    if(!s){fprintf(stderr,"bitmap resource %u: %s\n",id,SDL_GetError());return NULL;}
    SDL_SetColorKey(s,SDL_TRUE,SDL_MapRGB(s->format,0,0,0)); SDL_Texture *t=SDL_CreateTextureFromSurface(r,s); SDL_FreeSurface(s);
    if(t)SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND);
    return t;
}
static void draw_sprite(NativeGraphics *g, SDL_Texture *texture, const SniperSprite *sprite, int32_t x, int32_t y, int32_t frame, int centered) {
    if(!texture)return;
    SniperBlit b; if(sniper_sprite_blit(sprite,x,y,frame,centered,0,&b))return;
    SDL_Rect src={b.source.left,b.source.top,b.source.right-b.source.left,b.source.bottom-b.source.top};
    SDL_Rect dst={b.destination.left,b.destination.top,b.destination.right-b.destination.left,b.destination.bottom-b.destination.top};
    SDL_RenderCopy(g->renderer,texture,&src,&dst);
}
static void body_draw(void *u,int slot,int32_t x,int32_t y,int32_t frame) {
    NativeGraphics *g=u; if(slot<0||slot>=9)return; draw_sprite(g,g->ships[slot],&sniper_ship_assets[slot],x,y,frame,1);
}
static void effect_draw(void *u,int resource,int32_t x,int32_t y,int32_t frame) {
    NativeGraphics *g=u; const SniperSprite *s=resource==173?&sniper_effect_assets[1]:&sniper_effect_assets[0];
    draw_sprite(g,g->ships[resource==173?5:6],s,x,y,frame,1);
}
static void label_draw(void *u,const char *text,int32_t x,int32_t y,int color) { (void)u;(void)text;(void)x;(void)y;(void)color; }
static void map_draw(void *u,unsigned tile,int32_t x,int32_t y,int32_t frame,int opaque) {
    NativeGraphics *g=u; if(tile<216||tile>=220)return; int i=(int)tile-216;
    draw_sprite(g,g->tiles[i],&sniper_large_tiles[i],x,y,frame,0); (void)opaque;
}
static void draw_recovered_scene(NativeGraphics *g,SniperOfflineSession *s,SniperShipObject *local) {
    SniperCamera cam=s->frame.camera;
    sniper_map_draw(s->map,cam.x,cam.y,800,600,(int32_t)s->frame.clock.tick,map_draw,g);
    uint32_t blink=s->frame.clock.tick;
    for(int i=0;i<s->ships.player_count;i++) {
        SniperShipObject *ship=s->ordered[i]; SniperShipObject *parent=NULL;
        int id=ship->word[0x6c/4]; if(id>=0&&id<1024)parent=s->ships.players[id];
        sniper_ship_render(ship,local,parent,&blink,0,s->label_mode,0,0,0,cam.x,cam.y,effect_draw,body_draw,label_draw,g);
    }
}
int main(int argc, char **argv) {
    const char *level=argc>1?argv[1]:"LOCAL.LVL"; uint8_t *level_data; size_t level_bytes; load_level(level,&level_data,&level_bytes);
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0){fprintf(stderr,"SDL: %s\n",SDL_GetError());return 2;}
    SDL_Window *window=SDL_CreateWindow("Sniper 87 reconstructed (native Linux)",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,800,600,0);
    SDL_Renderer *renderer=window?SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE):NULL;
    if(!window||!renderer){fprintf(stderr,"SDL window: %s\n",SDL_GetError());return 2;}
    NativeGraphics graphics={.renderer=renderer};
    /* Bitmaps come from the generated sniper_assets.c (make -C .. assets). */
    const unsigned ship_resources[9]={700,700,700,700,700,141,178,158,172};
    for(int i=0;i<9;i++)graphics.ships[i]=load_texture(renderer,ship_resources[i]);
    const unsigned tile_resources[4]={154,155,156,157};
    for(int i=0;i<4;i++)graphics.tiles[i]=load_texture(renderer,tile_resources[i]);
    NativeInput input={0,0}; SniperSessionHost host={.clock_ms=clock_ms,.key_state=key_state,.joystick=joystick,.post_command=post_command,.cursor=cursor,.user=&input,.key_state_event=key_state};
    SniperTileSize sizes[256]; memset(sizes,0,sizeof sizes); for(int i=216;i<256;i++)sizes[i]=(SniperTileSize){16,16};
    SniperOfflineSession *session=sniper_session_create(&host,level_data,level_bytes,sizes,"LinuxPilot",0,0,800,600,1);
    free(level_data); if(!session){fprintf(stderr,"reconstructed session could not load %s\n",level);return 3;}
    int running=1; uint32_t frames=0; uint32_t last=SDL_GetTicks();
    while(running){
        SDL_Event e; input.event_key=0;
        while(SDL_PollEvent(&e)) { if(e.type==SDL_QUIT)running=0; if(e.type==SDL_KEYDOWN&&!e.key.repeat){
            input.event_key=virtual_key(e.key.keysym.sym); session->lifecycle.key=input.event_key;
            if(e.key.keysym.sym==SDLK_ESCAPE)running=0;
            if(e.key.keysym.sym==SDLK_UP)input.keys|=2;
            if(e.key.keysym.sym==SDLK_DOWN)input.keys|=8;
            if(e.key.keysym.sym==SDLK_LEFT)input.keys|=1;
            if(e.key.keysym.sym==SDLK_RIGHT)input.keys|=4;
            if(e.key.keysym.sym==SDLK_LSHIFT||e.key.keysym.sym==SDLK_RSHIFT)input.keys|=16;
        } if(e.type==SDL_KEYUP){if(e.key.keysym.sym==SDLK_UP)input.keys&=~2u;if(e.key.keysym.sym==SDLK_DOWN)input.keys&=~8u;if(e.key.keysym.sym==SDLK_LEFT)input.keys&=~1u;if(e.key.keysym.sym==SDLK_RIGHT)input.keys&=~4u;if(e.key.keysym.sym==SDLK_LSHIFT||e.key.keysym.sym==SDLK_RSHIFT)input.keys&=~16u;}}
        if(!sniper_session_step(session)){fprintf(stderr,"session stopped at frame %u\n",frames);break;}
        SniperShipObject *local=sniper_session_local(session);
        SDL_SetRenderDrawColor(renderer,3,7,17,255);SDL_RenderClear(renderer);SDL_SetRenderDrawColor(renderer,12,32,56,255);
        for(int x=0;x<800;x+=40)SDL_RenderDrawLine(renderer,x,0,x,600);
        for(int y=0;y<600;y+=40)SDL_RenderDrawLine(renderer,0,y,800,y);
        draw_recovered_scene(&graphics,session,local);
        char title[180];snprintf(title,sizeof title,"Sniper 87 native Linux | frame %u | ships %d | pos %d,%d | energy %d",frames,session->ships.player_count,local?local->word[0x48/4]:0,local?local->word[0x4c/4]:0,local?local->word[0x5c/4]:0);SDL_SetWindowTitle(window,title);SDL_RenderPresent(renderer);
        frames++;uint32_t now=SDL_GetTicks();if(now-last<16)SDL_Delay(16-(now-last));last=now;
    }
    printf("native linux frames=%u ships=%d\n",frames,session->ships.player_count);sniper_session_destroy(session);
    for(int i=0;i<9;i++)if(graphics.ships[i])SDL_DestroyTexture(graphics.ships[i]);
    for(int i=0;i<4;i++)if(graphics.tiles[i])SDL_DestroyTexture(graphics.tiles[i]);
    SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 0;
}
