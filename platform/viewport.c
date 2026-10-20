#include "viewport.h"
#include <stdlib.h>
HRESULT sniper_viewport_clip(LPDIRECTDRAW dd,LPDIRECTDRAWSURFACE surface,int w,int h){
    if(w<=0 || h<=0)return E_INVALIDARG;
    LPDIRECTDRAWCLIPPER clipper=NULL;
    HRESULT hr=IDirectDraw_CreateClipper(dd,0,&clipper,NULL);
    if(FAILED(hr))return hr;
    HRGN region=CreateRectRgn(0,0,w,h);
    if(!region){IDirectDrawClipper_Release(clipper);return E_FAIL;}
    DWORD bytes=GetRegionData(region,0,NULL);
    RGNDATA *data=bytes ? malloc(bytes) : NULL;
    if(!data || !GetRegionData(region,bytes,data))hr=E_FAIL;
    else hr=IDirectDrawClipper_SetClipList(clipper,data,0);
    free(data);DeleteObject(region);
    if(SUCCEEDED(hr))hr=IDirectDrawSurface_SetClipper(surface,clipper);
    IDirectDrawClipper_Release(clipper);
    return hr;
}
