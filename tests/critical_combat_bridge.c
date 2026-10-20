/* Persistent production-core fixture. Only allocation, audio and clock are host seams. */
#include "offline_combat.h"
#include "offline_simulation.h"
#include "local_control.h"
#include "offline_session.h"
#include "numeric_environment.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#define CAP 1024
static SniperOfflineSession *session;
#define sw (session->ships)
#define combat (session->combat)
#define grid_map (session->map)
#define ordered (session->ordered)
static SniperShipObject ships[2];
static SniperProjectile pool[CAP];
static int32_t allocated,events[16384],event_count,bits,dt;
static uint32_t milliseconds,clock_calls;
static void event(int a,int b,int c,int d){if(event_count>=4096)abort();int *p=events+4*event_count++;p[0]=a;p[1]=b;p[2]=c;p[3]=d;}
static SniperProjectile *allocate(void *p){(void)p;if(allocated>=CAP)abort();event(1,allocated,0,0);return pool+allocated++;}
static void release(void *p,SniperProjectile *v){(void)p;event(2,(int)(v-pool),0,0);}
static void sound(void *p,int32_t h,int32_t x,int32_t y){(void)p;event(3,h,x,y);}
static void audio(void *p,int32_t h,const SniperAudioRequest *r){(void)p;event(4,h,r->volume,r->pan);event(5,r->loop,r->pitch,0);}
static uint32_t clock_ms(void *p){(void)p;return milliseconds+7*clock_calls++;}
static int32_t key(void *p,int32_t vk){(void)p;int mask=vk==17?1:vk==16?2:vk==9?4:0;return bits&mask?0x8000:0;}
void critical_init(const void *initial,const void *config,const void *grid,uint32_t seed){
 unsigned short control=0x037f;__asm__ volatile("fldcw %0"::"m"(control));
 sniper_numeric_initialize();__asm__ volatile("fnstcw %0":"=m"(control));assert(control==0x027f);
 free(session);SniperSessionHost host={.clock_ms=clock_ms,.key_state=key,.sound=sound,.audio=audio};session=sniper_session_prepare(&host,640,480,seed);if(!session)abort();session->start.local_id=0;
 memcpy(ships,initial,sizeof ships);memcpy(grid_map,grid,sizeof grid_map);memcpy(&sw.config,config,sizeof sw.config);memset(pool,0x55,sizeof pool);
 allocated=event_count=0;sw.players[0]=ordered[0]=ships;sw.players[1]=ordered[1]=ships+1;sw.player_count=2;sw.random_state=seed;sw.offline_backend=1;sw.movement.map=grid_map;sw.movement.local_id=0;
 combat.ships=&sw;combat.ordered_ships=ordered;combat.next_group=123;combat.allocate=allocate;combat.release=release;
}
static void input_stage(void *user,int stage){(void)user;if(stage==1){SniperWeaponInput input={0};input.key_state=key;sniper_offline_combat_input(&combat,&input,dt);}}
void critical_input(int32_t ticks,uint32_t ms,int32_t keys){dt=ticks;milliseconds=ms;clock_calls=0;sw.milliseconds=ms;bits=keys;event_count=0;
 SniperLocalControl life={.local=ships,.stage=input_stage};sniper_local_control(&life);
}
void critical_weapon(const int32_t args[14]){sniper_offline_combat_weapon(&combat,args);}
void critical_attach(int32_t target){sniper_ship_world_offline_attach(&sw,target);}
void critical_simulate(void){sniper_offline_simulation(&combat,NULL,&dt);session->frame.camera=sniper_camera(ships,NULL,640,480,0,0,0,combat.shake,&sw.random_state);}
void critical_copy(int which,void *out){
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
 }
}
