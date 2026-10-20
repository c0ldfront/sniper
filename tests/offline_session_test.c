#include "offline_session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {uint32_t now;int frame,cursors,posts,sounds;} Host;
static int has_notice(const SniperAttachmentHud *n,const char *text){
    for(int i=0;i<4;++i)if(n->ticks[i]>0&&!strcmp(n->text[i],text))return 1;
    return 0;
}
static int32_t event_shift;
static int32_t event_key(void *p,int32_t vk){(void)p;return vk==16?event_shift:0;}
static unsigned clock_calls;
static uint32_t clock_ms(void *p){++clock_calls;return ((Host *)p)->now;}
static int32_t key(void *p,int32_t vk){int f=((Host *)p)->frame;return ((vk==38&&f%200<120)||(vk==39&&f%160<50)||(vk==17&&f%5==0))?0x8000:0;}
static void cursor(void *p,int32_t x,int32_t y){if(x==0&&y==0)++((Host *)p)->cursors;}
static void post(void *p){++((Host *)p)->posts;}
static void sound(void *p,int32_t h,int32_t x,int32_t y){(void)h;(void)x;(void)y;++((Host *)p)->sounds;}
#define REQUIRE(c) do{if(!(c)){printf("FAIL line%d frame%d\n",__LINE__,a.frame);return 1;}}while(0)
int main(int argc,char **argv) {
    Host a={1000,0,0,0,0},b=a;
    if(argc!=2)return 2;
    FILE *file=fopen(argv[1],"rb");if(!file)return 2;
    fseek(file,0,SEEK_END);long length=ftell(file);rewind(file);
    if(length<0)return 2;
    uint8_t *bytes=malloc((size_t)length);if(!bytes)return 2;
    if(fread(bytes,1,(size_t)length,file)!=(size_t)length)return 2;
    fclose(file);
    SniperTileSize sizes[256]={{0}};
    for(int i=0;i<4;++i){int side=(int[]){16,32,64,96}[i];sizes[216+i]=(SniperTileSize){side,side};}
    SniperSessionHost ha={.clock_ms=clock_ms,.key_state=key,.post_command=post,.cursor=cursor,.sound=sound,.user=&a,.key_state_event=event_key},hb=ha;hb.user=&b;
    SniperOfflineSession *s=sniper_session_create(&ha,bytes,(size_t)length,sizes,"Pilot",0,1,640,480,87);
    SniperOfflineSession *t=sniper_session_create(&hb,bytes,(size_t)length,sizes,"Pilot",0,1,640,480,87);
    free(bytes);REQUIRE(s&&t);REQUIRE(a.cursors==1&&b.cursors==1);
    REQUIRE(s->label_mode==1&&t->label_mode==1);
    REQUIRE(s->stars_enabled==1&&t->stars_enabled==1);
    /* Ordinary display keys act this frame; shifted macro/voice commands
     * consume them before the display switch. Return toggles to defaults. */
    for(int pass=0;pass<2;++pass) {
        event_shift=pass?0:0x8000;
        for(int c=0;c<3;++c) {
            const int display_keys[3]={0x72,0x75,0x7a};
            s->lifecycle.key=t->lifecycle.key=display_keys[c];
            REQUIRE(sniper_session_step(s)&&sniper_session_step(t));
        }
        REQUIRE(s->label_mode==(pass?2:1)&&t->label_mode==s->label_mode);
        REQUIRE(s->stars_enabled==!pass&&t->stars_enabled==s->stars_enabled);
        REQUIRE(s->ticker_enabled==!pass&&t->ticker_enabled==s->ticker_enabled);
    }
    for(int c=0;c<4;++c) {
        const int restore[4]={0x72,0x72,0x75,0x7a};
        s->lifecycle.key=t->lifecycle.key=restore[c];
        REQUIRE(sniper_session_step(s)&&sniper_session_step(t));
    }
    REQUIRE(s->label_mode==1&&s->stars_enabled==1&&s->ticker_enabled==1);
    /* Command modifiers use queued state, distinct from the async callback
     * (which never reports Shift in this fixture). No time elapses here. */
    const int commands[5][4]={{0x24,0,0xec,0x88},{0x24,0x8000,0xf0,0x8c},
        {0x23,0,0xf8,0x94},{0x23,0x8000,0xf4,0x90},{0x2e,0,0xfc,0x98}};
    for(int c=0;c<5;++c) {
        event_shift=commands[c][1];
        for(int pass=0;pass<2;++pass) {
            s->ships.players[0]->word[commands[c][2]/4]=t->ships.players[0]->word[commands[c][2]/4]=1;
            s->lifecycle.key=t->lifecycle.key=commands[c][0];
            REQUIRE(sniper_session_step(s)&&sniper_session_step(t));
            REQUIRE(s->ships.players[0]->word[commands[c][3]/4]==!pass);
            REQUIRE(t->ships.players[0]->word[commands[c][3]/4]==!pass);
            REQUIRE(!s->lifecycle.key&&!t->lifecycle.key);
        }
    }
    event_shift=0;
    s->binding.selected=t->binding.selected=3;
    event_shift=0;
    s->lifecycle.key=t->lifecycle.key=0x21;
    REQUIRE(sniper_session_step(s)&&sniper_session_step(t));
    REQUIRE(s->binding.selected==2&&t->binding.selected==2);
    s->lifecycle.key=t->lifecycle.key=0x22;
    REQUIRE(sniper_session_step(s)&&sniper_session_step(t));
    REQUIRE(s->binding.selected==0&&t->binding.selected==0);
    event_shift=0x8000;
    s->lifecycle.key=t->lifecycle.key=0x21;
    REQUIRE(sniper_session_step(s)&&sniper_session_step(t));
    REQUIRE(s->start.history_size==19&&t->start.history_size==19);
    s->lifecycle.key=t->lifecycle.key=0x22;
    REQUIRE(sniper_session_step(s)&&sniper_session_step(t));
    REQUIRE(s->start.history_size==20&&t->start.history_size==20);
    REQUIRE(s->binding.list_dirty&&t->binding.list_dirty);
    /* Shift-F7 belongs to macro handling, so must not attach. */
    event_shift=0x8000;s->lifecycle.key=t->lifecycle.key=0x76;
    REQUIRE(sniper_session_step(s)&&sniper_session_step(t));
    REQUIRE(!has_notice(&s->ships.attachment_hud,"Cannot attach to self."));
    event_shift=0;
    s->ships.players[0]->word[0x5c/4]=s->ships.players[0]->word[0xc8/4];
    t->ships.players[0]->word[0x5c/4]=t->ships.players[0]->word[0xc8/4];
    s->lifecycle.key=t->lifecycle.key=0x76;
    REQUIRE(sniper_session_step(s)&&sniper_session_step(t));
    REQUIRE(has_notice(&s->ships.attachment_hud,"Cannot attach to self."));
    REQUIRE(!s->lifecycle.key&&!t->lifecycle.key);
    for(int success=0;success<2;++success) {
        s->ships.players[0]->word[0x5c/4]=s->ships.players[0]->word[0xc8/4]-(1-success);
        t->ships.players[0]->word[0x5c/4]=t->ships.players[0]->word[0xc8/4]-(1-success);
        uint32_t previous=s->ships.random_state;int sounds=a.sounds;
        s->lifecycle.key=t->lifecycle.key=0x2d;
        REQUIRE(sniper_session_step(s)&&sniper_session_step(t));
        if(success) {
            REQUIRE(s->ships.players[0]->word[0x5c/4]==0);
            REQUIRE(s->ships.random_state!=previous&&a.sounds==sounds+1);
        } else {
            REQUIRE(has_notice(&s->ships.attachment_hud,"Not enough energy to warp."));
            REQUIRE(s->ships.random_state==previous&&a.sounds==sounds);
        }
        REQUIRE(!s->lifecycle.key&&!t->lifecycle.key);
    }
    int32_t x=s->ships.players[0]->word[0x48/4],y=s->ships.players[0]->word[0x4c/4];
    int moved=0,shots=0,pickups=0,died=0,respawned=0;
    for(a.frame=b.frame=0;a.frame<1800;++a.frame,++b.frame) {
        a.now+=10;b.now+=10;
        if(a.frame==300)s->lifecycle.key=t->lifecycle.key=123;
        REQUIRE(sniper_session_step(s)&&sniper_session_step(t));
        REQUIRE(!memcmp(s->ships.players[0],t->ships.players[0],sizeof(SniperShipObject)));
        REQUIRE(s->ships.random_state==t->ships.random_state);
        REQUIRE(s->combat.count==t->combat.count&&s->pickups.collection.count==t->pickups.collection.count);
        for(int i=0;i<s->combat.count;++i)REQUIRE(!memcmp(s->combat.projectiles[i],t->combat.projectiles[i],sizeof(SniperProjectile)));
        for(int i=0;i<s->pickups.collection.count;++i)REQUIRE(!memcmp(s->pickup_items[i],t->pickup_items[i],sizeof(SniperPickupObject)));
        REQUIRE(!memcmp(&s->frame.camera,&t->frame.camera,sizeof(SniperCamera)));
        moved|=s->ships.players[0]->word[0x48/4]!=x||s->ships.players[0]->word[0x4c/4]!=y;
        shots|=s->combat.count>0;pickups|=s->pickups.collection.count>0;
        died|=s->ships.players[0]->word[0x5c/4]<0;
        respawned|=died&&s->ships.players[0]->word[0x5c/4]>0;
    }
    REQUIRE(moved&&shots&&pickups&&died&&respawned);
    REQUIRE(!memcmp(s->map,t->map,SNIPER_MAP_BYTES));
    REQUIRE(a.sounds==b.sounds&&a.posts==b.posts);
    sniper_session_destroy(s);sniper_session_destroy(t);sniper_session_destroy(NULL);
    SniperFrameClock process={777,23,999};ha.process_clock=&process;a.now=10000;
    for(int cycle=0;cycle<2;++cycle){
        SniperFrameClock previous=process;
        s=sniper_session_create(&ha,NULL,0,sizes,"Clock",0,0,640,480,87);REQUIRE(s);
        REQUIRE(s->frame.clock.previous==previous.previous&&s->frame.clock.elapsed==previous.elapsed);
        REQUIRE(s->frame.clock.tick==0&&process.tick==0);
        unsigned reads=clock_calls;sniper_session_bind_local(s,s->ships.players[0],0);
        REQUIRE(clock_calls==reads);
        REQUIRE(sniper_session_step(s));REQUIRE(s->frame.clock.elapsed==50&&s->frame.clock.tick==50);
        REQUIRE(!memcmp(&process,&s->frame.clock,sizeof process));
        previous=s->frame.clock;sniper_session_destroy(s);
        REQUIRE(!memcmp(&process,&previous,sizeof process));a.now+=10000;
    }
    uint32_t process_random=87;ha.process_random_state=&process_random;
    for(int cycle=0;cycle<3;++cycle){
        uint32_t seed=process_random;
        s=sniper_session_create(&ha,NULL,0,sizes,"Random",0,0,640,480,12345);REQUIRE(s);
        t=sniper_session_create(&hb,NULL,0,sizes,"Random",0,0,640,480,seed);REQUIRE(t);
        REQUIRE(!memcmp(&s->stars,&t->stars,sizeof s->stars));
        REQUIRE(s->ships.random_state==t->ships.random_state);
        /* Teardown must also publish a final draw occurring after last step. */
        sniper_random(&s->ships.random_state);uint32_t expected=s->ships.random_state;
        sniper_session_destroy(s);sniper_session_destroy(t);REQUIRE(process_random==expected);
    }
    /* Invalid map position fails after starfield generation inside constructor. */
    uint32_t before_failure=process_random;
    t=sniper_session_prepare(&hb,640,480,before_failure);REQUIRE(t);
    uint32_t after_failure=t->ships.random_state;sniper_session_destroy(t);
    const uint8_t invalid_map[4]={255,15,0,1};
    REQUIRE(!sniper_session_create(&ha,invalid_map,4,sizes,"Fail",0,0,640,480,12345));
    REQUIRE(process_random==after_failure&&process_random!=before_failure);
    puts("PASS shared CRT stream survives restart and constructor failure; explicit seed remains standalone fallback");
    puts("PASS process clock survives teardown/restart;session tick resets;first elapsed includes idle time;bind-local takes no extra sample");
    printf("PASS queued capability toggles/navigation/attachment/warp plus1800 persistent frames: movement,firing,generation,self-destruct,respawn,deterministic paired state and cleanup\n");
    return 0;
}
