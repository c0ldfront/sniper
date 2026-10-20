#include "ship_assets.h"
/* Original sprite construction403302: five128px-high slices of bitmap700.
 * Slot4 is the field84 override, not a fifth selectable ship. */
const SniperSprite sniper_ship_assets[9]={
    {700,0,0,32,32,4,10,0,40},
    {700,0,128,32,32,4,10,0,40},
    {700,0,256,32,32,4,10,0,40},
    {700,0,384,32,32,4,10,0,40},
    {700,0,512,32,32,4,10,0,40},
    {141,0,0,48,48,6,6,102,34},
    {178,0,0,48,48,3,6,48,16},
    {158,0,0,40,40,5,8,0,40},
    {172,0,0,16,16,2,20,0,40}
};
const SniperSprite sniper_effect_assets[2]={
    {147,0,0,16,16,1,19,57,19},
    {173,0,0,16,16,2,20,52,26}
};
