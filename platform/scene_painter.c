#include "scene_painter.h"
static void stage(void *ctx,SniperSceneStage value)
{
    SniperScenePainter *p=ctx;if(FAILED(p->result))return;HRESULT hr=DD_OK;
    switch(value){
    case SNIPER_SCENE_STARS:
        if(!p->stars){hr=E_INVALIDARG;break;}
        hr=sniper_starfield_paint(p->stars,p->target,p->width,p->height,*p->camera_x,*p->camera_y,p->near_color,p->far_color);break;
    case SNIPER_SCENE_BATCH_BEGIN:case SNIPER_SCENE_BATCH_FLUSH:
        /* All bound sprite paths issue immediate Blt;reset count remains zero. */
        break;
    case SNIPER_SCENE_MAP:
        if(!p->map_painter||!p->map){hr=E_INVALIDARG;break;}
        hr=sniper_map_paint(p->map_painter,p->target,p->map,*p->camera_x,*p->camera_y,p->width,p->height,p->tick);break;
    case SNIPER_SCENE_ACTORS:
        if(!p->actors||!p->ship_order||!p->ship_options||!p->overlay||!p->projectiles||!p->ships){hr=E_INVALIDARG;break;}
        hr=sniper_actors_paint(p->actors,p->ship_order,p->ship_options,p->overlay,p->projectiles,p->ships,p->target,p->suppress_impacts);break;
    case SNIPER_SCENE_HUD:
        if(!p->hud||!p->hud_panels){hr=E_INVALIDARG;break;}
        p->hud->local=p->frame->local;p->hud->width=p->width;p->hud->height=p->height;
        hr=sniper_hud_paint(p->hud_panels,p->target,p->hud,p->hud_background,p->hud_highlight);break;
    case SNIPER_SCENE_NOTICES:
        if(!p->notices||!p->overlay){hr=E_INVALIDARG;break;}
        hr=sniper_notices_paint(p->overlay,p->target,p->notices,p->width,p->height);break;
    default:hr=p->extension?p->extension(p->user,value):E_NOTIMPL;break;
    }
    /* Original4024c3..402682 ignores draw HRESULTs and reaches Flip, whose
     * SURFACELOST branch restores the display. Do not turn transient loss
     * during drawing into a fatal error that bypasses that recovery path. */
    if(FAILED(hr)&&hr!=DDERR_SURFACELOST)p->result=hr;
}
static uint32_t present(void *ctx)
{
    SniperScenePainter *p=ctx;
    /* Stop portable retry loop on adapter failure;report stored HRESULT. */
    if(FAILED(p->result))return 0;
    if(p->present)return p->present(p->user);
    return (uint32_t)IDirectDrawSurface_Flip(p->primary,NULL,DDFLIP_WAIT);
}
static int recover(void *ctx)
{
    SniperScenePainter *p=ctx;int ok=p->recover?p->recover(p->user):0;
    if(!ok)p->result=DDERR_SURFACELOST;
    return ok;
}
HRESULT sniper_scene_paint(SniperScenePainter *p)
{
    if(!p||!p->frame||!p->frame->local||!p->target||!p->camera_x||!p->camera_y||(!p->primary&&!p->present))return E_INVALIDARG;
    SniperSceneFrame *f=p->frame;
    void (*saved_stage)(void *,SniperSceneStage)=f->stage;
    uint32_t (*saved_flip)(void *)=f->flip;int (*saved_restore)(void *)=f->restore;void *saved_user=f->user;
    p->result=DD_OK;f->stage=stage;f->flip=present;f->restore=recover;f->user=p;
    sniper_scene_frame(f);
    f->stage=saved_stage;f->flip=saved_flip;f->restore=saved_restore;f->user=saved_user;
    return p->result;
}
