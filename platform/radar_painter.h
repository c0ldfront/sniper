#ifndef SNIPER_RADAR_PAINTER_H
#define SNIPER_RADAR_PAINTER_H
#include "fill_painter.h"
#include "../src/radar_markers.h"
typedef struct {LPDIRECTDRAWSURFACE surface;int32_t scale,small,size;} SniperRadarPainter;
typedef struct {
    LPDIRECTDRAW draw;LPDIRECTDRAWSURFACE target;
    const uint8_t *map,*colors;
    int32_t width,height,camera_x,camera_y,scale,small;
    int32_t *dirty,*enabled;
    SniperRadarMarkers markers;
    int (*recover)(void *);void *user;
} SniperRadarPaint;
void sniper_radar_close(SniperRadarPainter *);
HRESULT sniper_radar_paint(SniperRadarPainter *,SniperRadarPaint *);
#endif
