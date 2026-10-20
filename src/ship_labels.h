#ifndef SNIPER_SHIP_LABELS_H
#define SNIPER_SHIP_LABELS_H
#include "ship_create.h"
typedef void (*SniperLabelDraw)(void *,const char *,int32_t,int32_t,int);
/* Screen x/y include body perturbation, as produced by preceding renderer.
 * attached_parent is resolved by target's attachment ID, or NULL.
 * Names/squad must be terminated in their32-byte fields. */
void sniper_ship_labels(const SniperShipObject *,const SniperShipObject *local,
    SniperShipObject *attached_parent,int mode,int32_t upper,int32_t lower,
    int32_t bonus,int32_t x,int32_t y,SniperLabelDraw,void *);
#endif
