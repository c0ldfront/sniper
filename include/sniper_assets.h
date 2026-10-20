#ifndef SNIPER_ASSETS_H
#define SNIPER_ASSETS_H
/* Original Sniper 87 resources. The data is not part of this source tree:
 * tools/extract_assets.py generates sniper_assets.c (and sniper_assets.rc for
 * the Win32 resource compiler) from your own copy of the game. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint16_t type_id;            /* numeric resource type, e.g. 2 = bitmap; 0 when named */
    const char *type_name;       /* named resource type such as "WAVE", or NULL */
    uint16_t id;                 /* numeric resource id; 0 when named */
    const char *name;            /* named resource id, or NULL */
    uint16_t language;           /* Windows language id, e.g. 1033 */
    uint32_t size;
    const unsigned char *data;   /* raw payload exactly as stored in SNIPER.EXE */
} SniperAsset;

extern const SniperAsset sniper_assets[];
extern const size_t sniper_asset_count;

/* Find a resource by numeric type (type_name NULL) or named type, and numeric id. */
const SniperAsset *sniper_asset_find(unsigned type_id, const char *type_name, unsigned id);

#endif
