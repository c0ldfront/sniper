/* Controlled COM failures exercise production sprite draw recovery without GPU. */
#include "sprite_surface.h"
#include <assert.h>
#include <stdio.h>
static int blits,source_restores,target_restores,busy;
static HRESULT blit_result,target_result;
static IDirectDrawSurface source,target;
static HRESULT WINAPI blit(LPDIRECTDRAWSURFACE self,LPRECT dst,LPDIRECTDRAWSURFACE src,LPRECT area,DWORD flags,LPDDBLTFX fx){
    (void)dst;(void)area;(void)flags;(void)fx;assert(self==&target&&src==&source);++blits;
    if(busy){--busy;return DDERR_WASSTILLDRAWING;}return blit_result;
}
static HRESULT WINAPI restore(LPDIRECTDRAWSURFACE self){
    if(self==&source){++source_restores;return E_FAIL;}assert(self==&target);++target_restores;return target_result;
}
int main(void){
    IDirectDrawSurfaceVtbl vtable={0};vtable.Blt=blit;vtable.Restore=restore;source.lpVtbl=&vtable;target.lpVtbl=&vtable;
    SniperSpriteSurface sprite={0};sprite.surface=&source;sprite.descriptor.width=16;sprite.descriptor.height=16;sprite.descriptor.columns=1;sprite.descriptor.rows=1;
    const HRESULT results[]={DD_OK,DDERR_SURFACELOST,E_FAIL,DDERR_INVALIDOBJECT};
    unsigned cases=0;
    for(unsigned b=0;b<4;b++)for(unsigned r=0;r<4;r++)for(int wait=0;wait<3;wait++){
        blits=source_restores=target_restores=0;busy=wait;blit_result=results[b];target_result=results[r];
        HRESULT got=sniper_surface_draw(&sprite,&target,0,0,0,0,0);
        assert(got==(blit_result==DDERR_SURFACELOST?target_result:blit_result));
        assert(blits==wait+1);assert(source_restores==(blit_result==DDERR_SURFACELOST));assert(target_restores==source_restores);cases++;
    }
    printf("PASS %u production sprite draw recovery cases: busy retries, source/target restore order and target HRESULT propagation\n",cases);return 0;
}
