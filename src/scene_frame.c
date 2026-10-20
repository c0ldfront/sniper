#include "scene_frame.h"
int sniper_scene_frame(SniperSceneFrame *f)
{
    if(f->stars)f->stage(f->user,SNIPER_SCENE_STARS);
    f->stage(f->user,SNIPER_SCENE_BATCH_BEGIN);
    f->stage(f->user,SNIPER_SCENE_MAP);
    if(f->special)f->stage(f->user,SNIPER_SCENE_SPECIAL);
    else{
        f->stage(f->user,SNIPER_SCENE_ACTORS);
        if(f->local->word[0x40/4]!=4)f->stage(f->user,SNIPER_SCENE_HUD);
    }
    f->stage(f->user,SNIPER_SCENE_BATCH_FLUSH);
    if(f->radar)f->stage(f->user,SNIPER_SCENE_RADAR);
    if(f->chat)f->stage(f->user,SNIPER_SCENE_CHAT);
    f->stage(f->user,SNIPER_SCENE_NOTICES);
    switch(f->mode){
        case 1:f->stage(f->user,SNIPER_SCENE_MODE_ONE);break;
        case 2:f->stage(f->user,SNIPER_SCENE_MODE_TWO);break;
        case 3:f->stage(f->user,SNIPER_SCENE_MODE_THREE);break;
        case 4:f->stage(f->user,SNIPER_SCENE_MODE_FOUR);break;
    }
    for(;;){
        uint32_t hr=f->flip(f->user);
        if(!hr)return 1;
        if(hr==UINT32_C(0x887601c2)&&!f->restore(f->user))return 0;
    }
}
