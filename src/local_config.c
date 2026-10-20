#include "local_config.h"
#include <string.h>
static const int32_t defaults[72]={
    20000, /* 0x4281c9 */
    30000, /* 0x4281cd */
    220000, /* 0x4281d1 */
    200000, /* 0x4281d5 */
    400000, /* 0x4281d9 */
    1000000, /* 0x4281dd */
    500, /* 0x4281e1 */
    6000, /* 0x4281e5 */
    20, /* 0x4281e9 */
    2000, /* 0x4281ed */
    3000, /* 0x4281f1 */
    25, /* 0x4281f5 */
    50, /* 0x4281f9 */
    125, /* 0x4281fd */
    250, /* 0x428201 */
    10, /* 0x428205 */
    500, /* 0x428209 */
    80, /* 0x42820d */
    800, /* 0x428211 */
    72, /* 0x428215 */
    200, /* 0x428219 */
    1000, /* 0x42821d */
    500, /* 0x428221 */
    333, /* 0x428225 */
    250, /* 0x428229 */
    250, /* 0x42822d */
    20, /* 0x428231 */
    3500, /* 0x428235 */
    2600000, /* 0x428239 */
    1000, /* 0x42823d */
    160, /* 0x428241 */
    10, /* 0x428245 */
    2000, /* 0x428249 */
    1000000, /* 0x42824d */
    166, /* 0x428251 */
    100, /* 0x428255 */
    3, /* 0x428259 */
    250, /* 0x42825d */
    300000, /* 0x428261 */
    166, /* 0x428265 */
    3, /* 0x428269 */
    5, /* 0x42826d */
    32, /* 0x428271 */
    10, /* 0x428275 */
    16, /* 0x428279 */
    20, /* 0x42827d */
    4, /* 0x428281 */
    1500, /* 0x428285 */
    15000, /* 0x428289 */
    15000, /* 0x42828d */
    8, /* 0x428291 */
    720000, /* 0x428295 */
    500, /* 0x428299 */
    64, /* 0x42829d */
    150, /* 0x4282a1 */
    900, /* 0x4282a5 */
    10000, /* 0x4282a9 */
    0, /* 0x4282ad */
    6, /* 0x4282b1 */
    8, /* 0x4282b5 */
    50, /* 0x4282b9 */
    50, /* 0x4282bd */
    35, /* 0x4282c1 */
    10, /* 0x4282c5 */
    5000, /* 0x4282c9 */
    150, /* 0x4282cd */
    512, /* 0x4282d1 */
    3, /* 0x4282d5 */
    0, /* 0x4282d9 */
    0, /* 0x4282dd */
    1, /* 0x4282e1 */
    1000, /* 0x4282e5 */
};
void sniper_local_config(SniperLocalConfig *c) { memcpy(c->word,defaults,sizeof defaults); }
#define VALUE(a) c->word[((a)-0x4281c9)/4]
void sniper_local_ship_defaults(const SniperLocalConfig *c,SniperShipDefaults *d) {
    *d=(SniperShipDefaults){VALUE(0x428251),VALUE(0x42824d),VALUE(0x428241),
        VALUE(0x428245),VALUE(0x428249),VALUE(0x428255),VALUE(0x428259),
        VALUE(0x42825d),VALUE(0x428261),VALUE(0x428265)};
}
void sniper_local_boost(const SniperLocalConfig *c,SniperBoostConfig *d) {
    *d=(SniperBoostConfig){VALUE(0x428285),VALUE(0x428231),VALUE(0x428235),
        VALUE(0x428259),VALUE(0x42825d)};
}
int32_t sniper_local_bounce(const SniperLocalConfig *c) { return VALUE(0x428271); }
