#include "manual_warp.h"
static int sound_count,sound_energy;
static void sound(void *user,int32_t handle) {
    SniperShipObject *s=user;if(handle==87)++sound_count;sound_energy=s->word[0x5c/4];
}
/* values: ms, RNG, dirty, count, sound calls, energy at sound, return */
void test_manual_warp(SniperShipObject *ship,uint8_t *map,SniperAttachmentHud *notices,int32_t values[7]) {
    SniperShipWorld world={0};sniper_local_config(&world.config);
    world.movement.map=map;world.milliseconds=(uint32_t)values[0];world.random_state=(uint32_t)values[1];
    world.position_dirty=values[2];world.player_count=values[3];world.attachment_hud=*notices;
    world.prize_sound=sound;world.backend=ship;world.warp_sound=87;sound_count=0;sound_energy=0;
    values[6]=sniper_manual_warp(&world,ship);values[1]=(int32_t)world.random_state;values[2]=world.position_dirty;
    values[4]=sound_count;values[5]=sound_energy;*notices=world.attachment_hud;
}
