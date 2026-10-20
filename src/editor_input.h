#ifndef SNIPER87_EDITOR_INPUT_H
#define SNIPER87_EDITOR_INPUT_H
#include <stdint.h>
#include "map.h"
typedef struct {
    int32_t x,y,tile,selection,x1,y1,x2,y2,clipboard_width,clipboard_height;
    uint8_t *clipboard;
} SniperEditor;
typedef struct {
    int32_t key,repeat,radar,radar_dirty;
    uint8_t character;
} SniperEditorInput;
typedef struct {
    int32_t (*async_key)(void *,int32_t);
    int32_t (*event_key)(void *,int32_t);
    void (*place)(void *,int32_t,int32_t,int32_t);
    void (*exit)(void *);
    void *user;
} SniperEditorHost;
/* Full409180. Host place must mutate map synchronously; native backend+10.
 * Returns0 only on guarded invalid state/allocation failure (native undefined).
 * Clipboard is owned by editor and destroyed with sniper_editor_close. */
int sniper_editor_input(SniperEditor *,SniperEditorInput *,uint8_t *,const SniperTileSize [256],const SniperEditorHost *);
void sniper_editor_close(SniperEditor *);
#endif
