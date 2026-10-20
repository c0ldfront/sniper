#include "flight_math.h"
#include <string.h>
static int32_t bits(uint32_t u) { int32_t s; memcpy(&s,&u,4); return s; }
/* Retain the x87 transcendental instructions rather than substituting libm
 * near integer truncation boundaries. Original CRT 0x416d18 truncates to a
 * signed 64-bit integer and returns its low DWORD. */
static int32_t trunc_x87(long double value) {
    unsigned short saved, truncating;
    int64_t result;
    __asm__ volatile("fnstcw %0" : "=m"(saved));
    truncating=saved | 0x0c00;
    __asm__ volatile("fldcw %1; fldt %2; fistpq %0; fldcw %3"
                     : "=m"(result) : "m"(truncating),"m"(value),"m"(saved) : "st");
    return bits((uint32_t)result);
}
static long double sine(long double value) {
    long double result;
    __asm__ volatile("fldt %1; fsin; fstpt %0" : "=m"(result) : "m"(value) : "st");
    return result;
}
static long double cosine(long double value) {
    long double result;
    __asm__ volatile("fldt %1; fcos; fstpt %0" : "=m"(result) : "m"(value) : "st");
    return result;
}
void sniper_flight_accelerate(int32_t heading,int32_t thrust,int32_t acceleration,
                              int32_t ticks,int32_t *vx,int32_t *vy) {
    int32_t amount=bits((uint32_t)thrust*(uint32_t)acceleration*(uint32_t)ticks);
    /* Binary64 constant at original VA0x420000; integer heading quantization. */
    long double angle=(long double)(heading/1000)*(double)0x1.41b2f769a7903p-3;
    int32_t dy=trunc_x87(cosine(angle)*bits(0u-(uint32_t)amount));
    int32_t dx=trunc_x87(sine(angle)*amount);
    *vx=bits((uint32_t)*vx+(uint32_t)dx);
    *vy=bits((uint32_t)*vy+(uint32_t)dy);
}
void sniper_flight_limit(int32_t maximum,int32_t *vx,int32_t *vy) {
    long double angle;
    /* Original loads vx then vy, thus atan2(vx,vy), not atan2(vy,vx). */
    __asm__ volatile("fildl %1; fildl %2; fpatan; fstpt %0"
                     : "=m"(angle) : "m"(*vx),"m"(*vy) : "st");
    int32_t x=trunc_x87(sine(angle)*maximum);
    int32_t y=trunc_x87(cosine(angle)*maximum);
    if (x<0) x=bits(0u-(uint32_t)x);
    if (y<0) y=bits(0u-(uint32_t)y);
    int32_t nx=bits(0u-(uint32_t)x),ny=bits(0u-(uint32_t)y);
    if (*vx<nx) *vx=nx;
    if (*vx>x) *vx=x;
    if (*vy<ny) *vy=ny;
    if (*vy>y) *vy=y;
}
