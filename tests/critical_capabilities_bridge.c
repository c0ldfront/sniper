/* Persistent production-core fixture. Only allocation, audio and clock are host seams. */
#include "offline_combat.h"
#include "offline_simulation.h"
#include "local_control.h"
#include "offline_session.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "numeric_environment.h"
#define CAP 1024
static SniperOfflineSession *session;
#define sw (session->ships)
#define combat (session->combat)
#define grid_map (session->map)
#define ordered (session->ordered)
static SniperShipObject ships[2];
static SniperProjectile pool[CAP];
static SniperPickupObject pickup_pool[128];static int pickup_allocated;
static int32_t allocated,events[16384],event_count,bits,dt;
static uint32_t milliseconds,clock_calls;
static void event(int a,int b,int c,int d){if(event_count>=4096)abort();int *p=events+4*event_count++;p[0]=a;p[1]=b;p[2]=c;p[3]=d;}
static SniperProjectile *allocate(void *p){(void)p;if(allocated>=CAP)abort();event(1,allocated,0,0);return pool+allocated++;}
static void release(void *p,SniperProjectile *v){(void)p;event(2,(int)(v-pool),0,0);}
static void sound(void *p,int32_t h,int32_t x,int32_t y){(void)p;event(3,h,x,y);}
static void audio(void *p,int32_t h,const SniperAudioRequest *r){(void)p;event(4,h,r->volume,r->pan);event(5,r->loop,r->pitch,0);}
static void audio_stop(void *p,int32_t h){(void)p;event(9,h,0,0);}
static void post(void *p){(void)p;event(8,0,0,0);}
static SniperPickupObject *pickup_allocate(void *p){(void)p;if(pickup_allocated>=128)abort();event(6,pickup_allocated,0,0);return pickup_pool+pickup_allocated++;}
static void pickup_release(void *p,SniperPickupObject *v){(void)p;event(7,(int)(v-pickup_pool),0,0);}
static uint32_t clock_ms(void *p){(void)p;return milliseconds+7*clock_calls++;}
static int32_t key(void *p,int32_t vk){(void)p;int mask=vk==1?128:vk==17?1:vk==16?2:vk==9?4:vk==38?8:vk==40?16:vk==37?32:vk==39?64:0;return bits&mask?0x8000:0;}
void cap_init(const void *initial,const void *config,const void *grid,uint32_t seed){
 unsigned short cw=0x037f;__asm__ volatile("fldcw %0"::"m"(cw));sniper_numeric_initialize();__asm__ volatile("fnstcw %0":"=m"(cw));assert(cw==0x027f);
 free(session);SniperSessionHost host={.clock_ms=clock_ms,.key_state=key,.sound=sound,.audio=audio,.audio_stop=audio_stop,.post_command=post};session=sniper_session_prepare(&host,640,480,seed);if(!session)abort();session->start.local_id=0;
 memcpy(ships,initial,sizeof ships);memcpy(grid_map,grid,sizeof grid_map);memcpy(&sw.config,config,sizeof sw.config);memset(pool,0x55,sizeof pool);
 allocated=event_count=0;sw.players[0]=ordered[0]=ships;sw.players[1]=ordered[1]=ships+1;sw.player_count=2;sw.random_state=seed;sw.offline_backend=1;sw.movement.map=grid_map;sw.movement.local_id=0;
 combat.ships=&sw;combat.ordered_ships=ordered;combat.next_group=123;combat.allocate=allocate;combat.release=release;
 session->pickups.allocate=pickup_allocate;session->pickups.release=pickup_release;session->pickups.generation_tick=100;pickup_allocated=0;memset(pickup_pool,0,sizeof pickup_pool);
 sniper_offline_pickups_bind(&session->pickups);sniper_session_bind_local(session,ships,0);
 session->frame.clock.previous=100;session->frame.clock.elapsed=0;session->frame.clock.tick=0;session->binding.list_dirty=0;session->frame.frequency=0;
}
void cap_begin(int32_t ticks,uint32_t ms,int32_t keys){dt=ticks;milliseconds=ms;clock_calls=0;bits=keys;event_count=0;}
void cap_weapon(const int32_t args[14]){sniper_offline_combat_weapon(&combat,args);}
void cap_attach(int32_t target){sniper_ship_world_offline_attach(&sw,target);}
void cap_step(void){if(!sniper_session_step(session))abort();}
void cap_copy(int which,void *out){
 switch(which){
 case 0:memcpy(out,ships,sizeof ships);break;
 case 1:memcpy(out,pool,sizeof pool);break;
 case 2:memcpy(out,sw.coverage,sizeof sw.coverage);break;
 case 3:memcpy(out,&sw.attachment_hud,sizeof sw.attachment_hud);break;
 case 4:{int32_t meta[]={(int32_t)sw.random_state,combat.count,combat.next_group,combat.shake,allocated,event_count,combat.weapon_cooldown,combat.safe_timer,sw.position_dirty,combat.repel_timer,(int32_t)clock_calls};memcpy(out,meta,sizeof meta);break;}
 case 5:for(int i=0;i<512;++i)((int32_t *)out)[i]=combat.projectiles[i]?(int32_t)(combat.projectiles[i]-pool):-1;break;
 case 6:memcpy(out,events,(size_t)event_count*16);break;
 case 7:memcpy(out,grid_map,sizeof grid_map);break;
 case 8:memcpy(out,&session->frame.camera,sizeof session->frame.camera);break;
 case 9:{int32_t state[]={(int32_t)session->frame.clock.previous,(int32_t)session->frame.clock.elapsed,(int32_t)session->frame.clock.tick,session->frame.frequency,session->position.last_ticks,session->position.last_vx,session->position.last_vy,session->position.last_heading,session->control.exhaust_ticks,session->pickups.collection.count,(int32_t)session->pickups.generation_tick,pickup_allocated};memcpy(out,state,sizeof state);break;}
 case 10:memcpy(out,pickup_pool,sizeof pickup_pool);break;
 case 11:for(int i=0;i<16;++i)((int32_t *)out)[i]=session->pickup_items[i]?(int32_t)(session->pickup_items[i]-pickup_pool):-1;break;
 }
}

/* Mouse motion/button samples are host inputs, never output-state writes. */
void cap_mouse(int32_t x,int32_t y){session->mouse.x=x;session->mouse.y=y;}
/* Observe the real offline no-op position backend without changing dispatch. */
typedef struct {void (*send)(void *,const int32_t *);void *user;} PositionObserver;
static void observe_position(void *context,const int32_t args[13]){
    PositionObserver *o=context;int32_t defined[13];memcpy(defined,args,sizeof defined);
    if(!defined[5])memset(defined+6,0,5*sizeof *defined);
    const int32_t *raw=args;args=defined;
    event(10,args[0],args[1],args[2]);event(11,args[3],args[4],args[5]);
    event(12,args[6],args[7],args[8]);event(13,args[9],args[10],args[11]);event(14,args[12],0,0);
    o->send(o->user,raw);
}
int __real_sniper_position_dispatch(SniperPositionDispatch *,const int32_t [6]);
int __wrap_sniper_position_dispatch(SniperPositionDispatch *p,const int32_t weapon[6]){
    PositionObserver saved={p->send,p->user};p->send=observe_position;p->user=&saved;
    int result=__real_sniper_position_dispatch(p,weapon);p->send=saved.send;p->user=saved.user;return result;
}
int __real_sniper_weapon_action_fields(SniperWeaponActionWorld *,int,int32_t,int,int32_t *);
int __wrap_sniper_weapon_action_fields(SniperWeaponActionWorld *w,int active,int32_t action,int mine,int32_t *fields){
    event(15,active,action,mine);
    int result=__real_sniper_weapon_action_fields(w,active,action,mine,fields);
    int enabled=0;for(int i=0;i<5;i++)if(w->local->word[(0x88+4*i)/4])enabled|=1<<i;
    event(16,enabled,w->local->word[0x5c/4],w->position_dirty);return result;
}
