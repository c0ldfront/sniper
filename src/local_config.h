#ifndef SNIPER_LOCAL_CONFIG_H
#define SNIPER_LOCAL_CONFIG_H
#include "ship_reset.h"
#include "flight_boost.h"
/* DWORD values from original unaligned globals4281c9..4282e5.
 * Unknown fields retain address-based identity pending protocol recovery. */
typedef struct { int32_t word[72]; } SniperLocalConfig;
void sniper_local_config(SniperLocalConfig *);
void sniper_local_ship_defaults(const SniperLocalConfig *,SniperShipDefaults *);
void sniper_local_boost(const SniperLocalConfig *,SniperBoostConfig *);
int32_t sniper_local_bounce(const SniperLocalConfig *);
#endif
