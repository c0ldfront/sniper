#include "weapon_create.h"
#include <string.h>
static int32_t bits(uint32_t x){int32_t r;memcpy(&r,&x,4);return r;}
static int32_t add(int32_t a,int32_t b){return bits((uint32_t)a+(uint32_t)b);}
static void emit(SniperWeaponCreateWorld *w,const int32_t a[14],int32_t heading,int stationary){
    int32_t args[15]={a[12],a[8],a[9],stationary?0:a[10],stationary?0:a[11],heading,
        stationary?0:w->speed,a[0],a[1],a[2],a[3],a[4],*w->next_group,a[13],0};
    SniperProjectile *p=w->create(w->user,args);
    if(a[6] && w->update(w->user,p,a[6]))w->release(w->user,p);
    else w->items[(*w->count)++]=p;
}
void sniper_weapon_create(SniperWeaponCreateWorld *w,const int32_t a[14]){
    if(*w->count>=510)return;
    if(a[5] && (a[0]==1 || a[0]==2)){
        emit(w,a,a[7],0);
        int32_t left=add(bits((uint32_t)a[7]-(uint32_t)w->spread),40000)%40000;
        int32_t right=add(a[7],w->spread)%40000;
        emit(w,a,left,0);emit(w,a,right,0);
    }else{
        int stationary=a[5]!=0 || a[0]==5;
        emit(w,a,!a[5] && a[0]==5?0:a[7],stationary);
    }
    *w->next_group=add(*w->next_group,1);
}
