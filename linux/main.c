#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "flight_input.h"
#include "flight_math.h"
#include "map.h"
#include "movement.h"
#include "ship_energy.h"
#include "numeric_environment.h"

typedef struct { uint32_t keys; } Input;
enum {
    SNIPER_VK_LEFT = 37,
    SNIPER_VK_UP = 38,
    SNIPER_VK_RIGHT = 39,
    SNIPER_VK_DOWN = 40,
    SNIPER_VK_SHIFT = 16
};
static int32_t key_state(void *u, int32_t key) {
    Input *in=(Input *)u;
    if(key==SNIPER_VK_UP)return (in->keys&SNIPER_UP)?0x8000:0;
    if(key==SNIPER_VK_DOWN)return (in->keys&SNIPER_DOWN)?0x8000:0;
    if(key==SNIPER_VK_LEFT)return (in->keys&SNIPER_LEFT)?0x8000:0;
    if(key==SNIPER_VK_RIGHT)return (in->keys&SNIPER_RIGHT)?0x8000:0;
    if(key==SNIPER_VK_SHIFT)return (in->keys&SNIPER_SHIFT)?0x8000:0;
    return 0;
}
static void impact(void *u,int32_t x,int32_t y){(void)u;(void)x;(void)y;}
static void pickup(void *u,int32_t x,int32_t y,int32_t player,int32_t value,int local){
    (void)u;(void)x;(void)y;(void)player;(void)value;(void)local;
}
static int parse_frames(int argc,char **argv){
    for(int i=1;i+1<argc;i++)if(!strcmp(argv[i],"--frames"))return atoi(argv[i+1]);
    return 0;
}
static int parse_demo(int argc,char **argv){
    for(int i=1;i<argc;i++)if(!strcmp(argv[i],"--demo-input"))return 1;
    return 0;
}
int main(int argc,char **argv){
    int fixed=parse_frames(argc,argv), demo=parse_demo(argc,argv); if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0){fprintf(stderr,"SDL: %s\n",SDL_GetError());return 2;}
    SDL_Window *window=SDL_CreateWindow("Sniper87 Linux",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,800,600,0);
    SDL_Renderer *renderer=window?SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE):NULL;
    if(!window||!renderer){fprintf(stderr,"SDL window: %s\n",SDL_GetError());SDL_Quit();return 2;}
    sniper_numeric_initialize();
    uint8_t *map=calloc(1,SNIPER_MAP_BYTES); if(!map)return 2;
    SniperMotion motion={512000*16,512000*16,0,0}; SniperMoveShip ship={0,0,0,-1,&motion.x};
    SniperMoveWorld world={map,0,-1,0,0,256,NULL,0,pickup,impact,NULL};
    SniperFlightControl control={0,100,0,-1}; SniperShipEnergy energy={100000,10,100000,{0,0,0,0}}; Input input={0};
    int running=1; uint32_t frame=0; uint32_t last=SDL_GetTicks();
    while(running && (!fixed || (int)frame<fixed)){
        if(demo){
            /* A repeatable input script exercises thrust, turning, and boost. */
            input.keys = frame < 30 ? SNIPER_UP :
                         frame < 60 ? (SNIPER_UP|SNIPER_RIGHT) :
                         frame < 90 ? (SNIPER_UP|SNIPER_LEFT|SNIPER_SHIFT) : 0;
        }
        SDL_Event event; while(SDL_PollEvent(&event)){if(event.type==SDL_QUIT)running=0;if(event.type==SDL_KEYDOWN||event.type==SDL_KEYUP){int down=event.type==SDL_KEYDOWN;uint32_t bit=0;if(event.key.keysym.sym==SDLK_UP)bit=SNIPER_UP;if(event.key.keysym.sym==SDLK_DOWN)bit=SNIPER_DOWN;if(event.key.keysym.sym==SDLK_LEFT)bit=SNIPER_LEFT;if(event.key.keysym.sym==SDLK_RIGHT)bit=SNIPER_RIGHT;if(event.key.keysym.sym==SDLK_LSHIFT||event.key.keysym.sym==SDLK_RSHIFT)bit=SNIPER_SHIFT;if(bit){if(down)input.keys|=bit;else input.keys&=~bit;}if(event.key.keysym.sym==SDLK_ESCAPE&&down)running=0;}}
        SniperFlightInput flight={(uint32_t)input.keys,0,0,0}; int thrust=sniper_flight_input_polled(&control,&flight,1,key_state,&input);
        if(thrust) {
            sniper_flight_accelerate(control.heading,thrust,120,1,&motion.vx,&motion.vy);
        }
        sniper_flight_limit(12000,&motion.vx,&motion.vy);
        sniper_move(&motion,&ship,&world,1,0); sniper_ship_energy_step(&energy,1,(int32_t[4]){1,1,1,1});
        if(renderer){
            SDL_SetRenderDrawColor(renderer,4,8,18,255); SDL_RenderClear(renderer);
            SDL_SetRenderDrawColor(renderer,12,32,56,255);
            for(int gx=0;gx<800;gx+=40) SDL_RenderDrawLine(renderer,gx,0,gx,600);
            for(int gy=0;gy<600;gy+=40) SDL_RenderDrawLine(renderer,0,gy,800,gy);
            /* Keep the camera centered on the local ship, as the game does. */
            double angle=(double)control.heading*(2.0*3.141592653589793/40000.0);
            double dx=sin(angle), dy=-cos(angle), sx=cos(angle), sy=sin(angle);
            SDL_Point nose={400+(int)lround(dx*30),300+(int)lround(dy*30)};
            SDL_Point left={400+(int)lround(-dx*18-sx*18),300+(int)lround(-dy*18-sy*18)};
            SDL_Point right={400+(int)lround(-dx*18+sx*18),300+(int)lround(-dy*18+sy*18)};
            SDL_Point ship[4]={nose,left,right,nose};
            SDL_SetRenderDrawColor(renderer,70,190,255,255); SDL_RenderDrawLines(renderer,ship,4);
            SDL_SetRenderDrawColor(renderer,170,230,255,255);
            SDL_RenderDrawLine(renderer,left.x,left.y,400,300); SDL_RenderDrawLine(renderer,400,300,right.x,right.y);
            if(thrust){
                SDL_Point flame[3]={{left.x+(int)lround(dx*10),left.y+(int)lround(dy*10)},
                                    {right.x+(int)lround(dx*10),right.y+(int)lround(dy*10)},
                                    {400-(int)lround(dx*28),300-(int)lround(dy*28)}};
                SDL_SetRenderDrawColor(renderer,255,170,45,255); SDL_RenderDrawLines(renderer,flame,3);
            }
            /* A few fixed markers make the otherwise empty development arena visible. */
            SDL_SetRenderDrawColor(renderer,90,110,145,255);
            for(int i=0;i<6;i++){ int mx=90+i*125, my=90+(i%3)*190; SDL_Rect marker={mx-3,my-3,6,6}; SDL_RenderFillRect(renderer,&marker); }
            char title[160]; snprintf(title,sizeof(title),"Sniper87 Linux | frame %u | pos %d,%d | vel %d,%d | energy %d",frame,motion.x,motion.y,motion.vx,motion.vy,energy.energy); SDL_SetWindowTitle(window,title);
            SDL_RenderPresent(renderer);
        }
        frame++; uint32_t now=SDL_GetTicks();if(!fixed&&now-last<16)SDL_Delay(16-(now-last));last=now;
    }
    printf("linux simulation frames=%u position=%d,%d velocity=%d,%d energy=%d heading=%d\n",frame,motion.x,motion.y,motion.vx,motion.vy,energy.energy,control.heading);
    free(map);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 0;
}
