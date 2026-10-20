#include "offline_combat.h"
#include "offline_simulation.h"
#include "offline_control.h"
#include "offline_frame.h"
#include <string.h>
static SniperProjectile *pool;
static SniperPickupObject *pickup_pool;
static int32_t allocated,*events,event_count;
static void event(int32_t a,int32_t b,int32_t d,int32_t e){int i=event_count++*4;events[i]=a;events[i+1]=b;events[i+2]=d;events[i+3]=e;}
static SniperProjectile *allocate(void *unused){(void)unused;event(1,allocated,0,0);return &pool[allocated++];}
static void release(void *unused,SniperProjectile *p){(void)unused;event(2,(int32_t)(p-pool),0,0);}
static void sound(void *unused,int32_t handle,int32_t x,int32_t y){(void)unused;event(3,handle,x,y);}
static int32_t key(void *context,int32_t vk){
    const int32_t *args=context;int mask=vk==17?1:vk==16?2:vk==9?4:vk==38?8:vk==40?16:vk==37?32:64;
    return args[1]&mask?0x8000:0;
}
static int32_t *control_result;
static int32_t posted;
static SniperPickupObject generated[16];
static int32_t generated_count;
static SniperPickupObject *allocate_pickup(void *user){(void)user;event(5,generated_count,0,0);return &generated[generated_count++];}
static void post(void *user){(void)user;++posted;}
static uint32_t clock_ms(void *user){(void)user;return 1000;}
static uint32_t poll(void *user,uint32_t id,uint32_t info[13]) {
    (void)id;const int32_t *a=user;
    info[2]=(uint32_t)a[12];info[3]=(uint32_t)a[13];
    info[8]=(uint32_t)a[3];info[10]=(uint32_t)a[4];return 0;
}
static void release_pickup(void *unused,SniperPickupObject *p){(void)unused;event(4,(int32_t)(p-pickup_pool),0,0);}
/* values: ticks,seed,count,group,shake,allocated,event_count; indices all32. */
static void run_combat(SniperShipObject ships[2],SniperProjectile objects[32],uint8_t *map,
    uint8_t *coverage,SniperNotices *hud,int32_t values[10],int32_t indices[32],int32_t *log,const int32_t *weapon,SniperPickupObject *pickup_objects){
    SniperShipWorld sw={0};SniperOfflineCombat c={0};SniperShipObject *order[]={ships,ships+1};
    sniper_local_config(&sw.config);sw.players[0]=ships;sw.players[1]=ships+1;sw.player_count=2;
    sw.movement.map=map;sw.movement.local_id=0;sw.offline_backend=1;sw.random_state=(uint32_t)values[1];
    sw.milliseconds=1000;sw.attachment_hud=*hud;memcpy(sw.coverage,coverage,8192);
    c.ships=&sw;c.ordered_ships=order;c.count=values[2];c.next_group=values[3];c.shake=values[4];
    c.impact_sound=101;c.explosion_sound=102;c.death_sound=103;c.repel_sound=104;
    c.allocate=allocate;c.release=release;c.sound=sound;
    pool=objects;allocated=c.count;events=log;event_count=0;
    for(int i=0;i<c.count;++i)c.projectiles[i]=objects+i;
    SniperOfflinePickups pickups={0};SniperPickupObject *pickup_items[16]={0};SniperPickup movement_view[16];
    if(pickup_objects){pickup_pool=pickup_objects;pickups.ships=&sw;pickups.collection.items=pickup_items;pickups.collection.count=2;
        pickup_items[0]=pickup_objects;pickup_items[1]=pickup_objects+1;pickups.movement_view=movement_view;pickups.release=release_pickup;
        sniper_offline_pickups_bind(&pickups);}
    if(weapon && (weapon[0]==-96 || weapon[0]==-95 || weapon[0]==-94)) {
        SniperLocalControl life={0};life.local=ships;life.key=weapon[9];life.repeat=7;
        life.mouse_x_event=17;life.mouse_y_event=-5;life.character=65;
        life.joystick_enabled=weapon[2];life.poll_joystick=poll;life.user=(void *)weapon;
        for(int i=0;i<13;++i)life.joystick[i]=0x12345678;
        SniperPositionDispatch position={0};position.interval=sw.config.word[(0x428205-0x4281c9)/4];position.ticks=weapon[11];
        SniperOrdinaryControl control={0};control.lifecycle=&life;control.position=&position;
        control.input=(SniperWeaponInput){weapon[2],weapon[3],weapon[4],weapon[5],weapon[6],weapon[7],weapon[8],key,(void *)weapon};
        control.ticks=values[0];control.speed_exempt=1;control.exhaust_ticks=weapon[10];control.fields[3]=weapon[5];
        int ran;
        if(weapon[0]==-95 || weapon[0]==-94) {
            sw.config.word[(0x4282a9-0x4281c9)/4]=4;
            SniperFrameUpdate frame={0};frame.clock.previous=100u-(uint32_t)values[0];
            frame.frequency=weapon[11];frame.width=640;frame.height=480;frame.post_command=post;
            c.repel_timer=100;
            pickups.ships=&sw;pickups.collection.items=pickup_items;pickups.collection.count=0;
            pickups.movement_view=movement_view;pickups.release=release_pickup;
            pickups.clock_ms=clock_ms;pickups.generation_tick=weapon[0]==-94?100u-501u:100u;
            generated_count=0;memset(generated,0,sizeof generated);pickup_pool=generated;
            pickups.allocate=allocate_pickup;
            sniper_offline_pickups_bind(&pickups);
            SniperOfflineFrame binding={.frame=&frame,.combat=&c,
                .pickups=&pickups,.control=&control};
            posted=0;ran=sniper_offline_frame(&binding,1000);
            int32_t frame_values[]={binding.selected,binding.list_dirty,(int32_t)frame.clock.previous,
                (int32_t)frame.clock.elapsed,(int32_t)frame.clock.tick,frame.camera.x,frame.camera.y,
                posted,frame.frequency,c.repel_timer};
            memcpy(control_result+40,frame_values,sizeof frame_values);
            memcpy(control_result+50,generated,sizeof generated[0]);
            control_result[55]=pickups.collection.count;control_result[56]=(int32_t)pickups.generation_tick;
        } else ran=sniper_offline_control(&c,&control);
        int32_t result[]={control.exhaust_ticks,life.key,life.repeat,life.mouse_x_event,life.mouse_y_event,life.character,
            control.dispatched,position.last_ticks,position.last_vx,position.last_vy,position.last_heading,position.position_dirty,ran,control.effects.sound_count};
        memcpy(control_result,result,sizeof result);
        memcpy(control_result+14,control.effects.sound_slot,12);memcpy(control_result+17,control.effects.sound_play,12);
        memcpy(control_result+20,life.joystick,52);memcpy(control_result+33,control.fields,24);
    }
    else if(weapon && weapon[0]==-97)sniper_offline_simulation(&c,pickup_objects?&pickups:NULL,&values[0]);
    else if(weapon && weapon[0]==-98){
        SniperWeaponInput input={weapon[2],weapon[3],weapon[4],weapon[5],weapon[6],weapon[7],weapon[8],key,(void *)weapon};
        sniper_offline_combat_input(&c,&input,values[0]);
    }
    else if(weapon && weapon[0]==-99)sniper_offline_combat_action(&c,weapon[1],weapon[2],weapon[3]);
    else if(weapon)sniper_offline_combat_weapon(&c,weapon);
    else sniper_offline_combat_frame(&c,&values[0]);
    values[1]=(int32_t)sw.random_state;values[2]=c.count;values[3]=c.next_group;values[4]=c.shake;
    memcpy(coverage,sw.coverage,8192);
    if(pickup_objects)values[7]=pickups.collection.count;
    values[5]=allocated;values[6]=event_count;*hud=sw.attachment_hud;
    if(weapon && (weapon[0]==-99 || weapon[0]==-98 || weapon[0]==-96 || weapon[0]==-95 || weapon[0]==-94)){values[7]=c.weapon_cooldown;values[8]=c.safe_timer;values[9]=sw.position_dirty;}
    for(int i=0;i<32;++i)indices[i]=c.projectiles[i]?(int32_t)(c.projectiles[i]-objects):-1;
}

void test_offline_combat(SniperShipObject ships[2],SniperProjectile objects[32],uint8_t *map,
    uint8_t *coverage,SniperNotices *hud,int32_t values[10],int32_t indices[32],int32_t *log){
    run_combat(ships,objects,map,coverage,hud,values,indices,log,NULL,NULL);
}
void test_offline_combat_weapon(SniperShipObject ships[2],SniperProjectile objects[32],uint8_t *map,
    uint8_t *coverage,SniperNotices *hud,int32_t values[10],int32_t indices[32],int32_t *log,const int32_t args[14]){
    run_combat(ships,objects,map,coverage,hud,values,indices,log,args,NULL);
}

void test_offline_simulation_pickups(SniperShipObject ships[2],SniperProjectile objects[32],uint8_t *map,
    uint8_t *coverage,SniperNotices *hud,int32_t values[10],int32_t indices[32],int32_t *log,SniperPickupObject *pickups){
    const int32_t args[14]={-97};run_combat(ships,objects,map,coverage,hud,values,indices,log,args,pickups);
}

void test_offline_control(SniperShipObject ships[2],SniperProjectile objects[32],uint8_t *map,
    uint8_t *coverage,SniperNotices *hud,int32_t values[10],int32_t indices[32],int32_t *log,const int32_t args[14],int32_t result[64]) {
    control_result=result;run_combat(ships,objects,map,coverage,hud,values,indices,log,args,NULL);
}
