#include "player_team.h"
static int group(const SniperShipObject *s,const SniperShipObject *local) {
    return s->word[0x44/4]==local->word[0x44/4]?0:s->word[0x68/4]==local->word[0x68/4]?1:2;
}
static unsigned lower(unsigned c) {return c>='A'&&c<='Z'?c+32:c;}
int sniper_player_compare(const SniperShipObject *a,const SniperShipObject *b,const SniperShipObject *local) {
    int ga=group(a,local),gb=group(b,local);
    if(ga!=gb)return ga<gb?-1:1;
    const unsigned char *x=(const unsigned char *)a,*y=(const unsigned char *)b;
    for(;;++x,++y) {
        unsigned cx=lower(*x),cy=lower(*y);
        if(cx!=cy)return cx<cy?-1:1;
        if(!cx)return 0;
    }
}
static void swap(SniperShipObject **a,int x,int y) {SniperShipObject *s=a[x];a[x]=a[y];a[y]=s;}
static void sort(SniperShipObject **a,int lo,int hi,const SniperShipObject *local) {
    if(hi-lo+1<=8) {
        for(;hi>lo;--hi) {
            int max=lo;
            for(int i=lo+1;i<=hi;++i)if(sniper_player_compare(a[i],a[max],local)>0)max=i;
            swap(a,max,hi);
        }
        return;
    }
    swap(a,lo,lo+(hi-lo+1)/2);
    int left=lo,right=hi+1;
    for(;;) {
        do {++left;} while(left<=hi && sniper_player_compare(a[left],a[lo],local)<=0);
        do {--right;} while(right>lo && sniper_player_compare(a[right],a[lo],local)>=0);
        if(right<left)break;
        swap(a,left,right);
    }
    swap(a,lo,right);
    if(lo+1<right)sort(a,lo,right-1,local);
    if(left<hi)sort(a,left,hi,local);
}
void sniper_player_sort(SniperShipObject **a,int32_t count,const SniperShipObject *local) {
    if(count>1)sort(a,0,count-1,local);
}
void sniper_player_team(SniperPlayerTeam *w,int32_t id,int32_t team) {
    SniperShipObject *s=w->players[id];
    if(s)s->word[0x68/4]=team;
    w->dirty=1;
    int32_t selected_id=-1;
    if(w->selected<w->count)selected_id=w->ordered[w->selected]->word[0x44/4];
    sniper_player_sort(w->ordered,w->count,w->local);
    for(int32_t i=0;i<w->count;++i)if(w->ordered[i]->word[0x44/4]==selected_id){w->selected=i;break;}
    if(w->local->word[0x6c/4]==id)w->detach(w->user,-1);
    if(s==w->local && w->reset_on_team) {
        w->spawn(w->user,w->local,1,0);
        w->local->word[0x5c/4]=0;
    }
}
