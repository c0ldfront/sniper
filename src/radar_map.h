#ifndef SNIPER_RADAR_MAP_H
#define SNIPER_RADAR_MAP_H
#include <stdint.h>
#include <stddef.h>
typedef struct {int32_t source_x,source_y,size,destination_x,destination_y;} SniperRadarView;
int sniper_radar_map_size(int32_t screen_width,int32_t scale,int small,int32_t *size);
void sniper_radar_view(int32_t screen_width,int32_t screen_height,int32_t camera_x,
    int32_t camera_y,int32_t map_width,int32_t map_height,int small,SniperRadarView *);
/*40732d..4073b1,after caller clears the surface. Map is1024x1024. Original
 * coordinates include+1 and can write an extra row/pixel beyond visible extent.
 * Supply padded storage: bytes>=(height+1)*pitch+width+1. Writes preserve original
 * linear offsets,including row overlap at pitch==width. Caller copies only its
 * visible extent to the display;never point this at an unpadded surface lock. */
int sniper_radar_map_pixels(uint8_t *pixels,size_t bytes,int32_t pitch,int32_t width,
    int32_t height,const uint8_t map[1048576],uint8_t wall,uint8_t safe);
#endif
