#include "network_backend.h"
#include "ship_effect.h"
#include "chat_history.h"
#include "pickup_lifecycle.h"
#include <string.h>
static int32_t bits(uint32_t u){int32_t s;memcpy(&s,&u,4);return s;}
static int valid(int32_t id){return id>=0&&id<SNIPER_NETWORK_PLAYER_LIMIT;}
static int32_t mul1000(int32_t x){return bits((uint32_t)x*1000);}
static void correct(int32_t *position,int32_t *velocity,int32_t predicted){
 int32_t d=bits((uint32_t)predicted-(uint32_t)*position);
 int32_t magnitude=d<0?bits(0u-(uint32_t)d):d;
 if(magnitude>64000)*position=predicted;
 else *velocity=bits((uint32_t)*velocity+(uint32_t)(d/(magnitude>4000?60:180)));
}
void sniper_network_position(SniperNetworkPosition*w,const int32_t a[15]){
 if(!w||!w->players||!valid(a[1]))return;
 SniperShipObject*s=w->players[a[1]];if(!s)return;
 s->word[0x74/4]=(int32_t)((w->clock_ms?w->clock_ms(w->user):0)/10);
 uint32_t flags=(uint32_t)a[13];s->word[0x88/4]=flags&1;s->word[0x8c/4]=(flags>>1)&1;s->word[0x94/4]=(flags>>2)&1;s->word[0x90/4]=(flags>>3)&1;
 if(flags&16)s->word[0xbc/4]=48;
 if(a[14]&&s->word[0xa0/4]>a[14])sniper_ship_effect1(s);
 s->word[0xa0/4]=a[14];
 int32_t x=mul1000(a[2]),y=mul1000(a[3]),vx=a[4],vy=a[5],heading=mul1000(a[6]);
 if(a[7]&&w->weapon){int32_t weapon[14]={a[7],a[8],a[9],a[10],a[11],a[12],a[0],heading,x,y,vx,vy,a[1],s->word[0x68/4]};w->weapon(w->user,weapon);}
 if(w->predict)w->predict(w->user,s,a[0],&x,&y,&vx,&vy);
 else if(w->movement){
  SniperMoveShip ship={s->word[0x44/4],s->word[0x60/4],s->word[0x64/4],s->word[0x6c/4],&s->word[0x48/4]};
  sniper_move_fields(&x,&y,&vx,&vy,&ship,w->movement,a[0],0);
 }
 s->word[0x50/4]=vx;s->word[0x54/4]=vy;s->word[0x58/4]=heading;
 correct(&s->word[0x48/4],&s->word[0x50/4],x);correct(&s->word[0x4c/4],&s->word[0x54/4],y);
}
static void extension(SniperNetworkBackend*b,const SniperGameEvent*e){if(b->extension)b->extension(b->user,e);}
void sniper_network_backend_event(void*user,const SniperGameEvent*e){
 SniperNetworkBackend*b=user;if(!b||!e)return;
 SniperPlayerRegistry*r=b->registry;const int32_t*a=e->args;
 if(!r){extension(b,e);return;}
 switch(e->kind){
 case SNIPER_NET_JOIN:{
  if(!valid(a[0])||r->count>=SNIPER_NETWORK_PLAYER_LIMIT||!e->data||!e->data2||!memchr(e->data,0,e->size)||!memchr(e->data2,0,e->size2))break;
  SniperShipIdentity id={a[0],(const char*)e->data,(const char*)e->data2,a[3],a[4],a[5],a[6],a[7],a[8],a[9]};
  uint32_t (*saved_clock)(void *)=r->clock_ms;void *saved_clock_user=r->clock_user;
  if(!r->clock_ms){r->clock_ms=b->clock_ms;r->clock_user=b->user;}
  sniper_player_add(r,&id);r->clock_ms=saved_clock;r->clock_user=saved_clock_user;return;
 }
 case SNIPER_NET_LEAVE:if(valid(a[0])){sniper_player_remove(r,a[0]);return;}break;
 case SNIPER_NET_POSITION:if(b->position&&(b->position->predict||b->position->movement)&&(!a[7]||b->position->weapon)){b->position->players=r->players;sniper_network_position(b->position,a);return;}break;
 case SNIPER_NET_DEATH:
  if(b->death&&r->local&&b->local_id&&valid(a[0])&&valid(a[1])){b->death->players=r->players;b->death->local=r->local;b->death->local_id=*b->local_id;sniper_death_event(b->death,a[0],a[1],a[2],a[3]);return;}break;
 case SNIPER_NET_CHAT:
  if(b->chat_lines&&b->chat_colors&&b->chat_rows&&b->screen_width&&b->chat_dirty&&e->data){
   const char*name=valid(a[1])?(const char*)r->players[a[1]]:NULL;
   sniper_chat_history(b->chat_lines,b->chat_colors,*b->chat_rows,*b->screen_width,b->chat_dirty,(const char*)e->data,name,a[2]);return;
  }break;
 case SNIPER_NET_PICKUP:
  if(b->pickups&&r->local&&valid(a[2])&&a[0]>=0&&a[0]<1024&&a[1]>=0&&a[1]<1024){b->pickups->local=r->local;sniper_pickup_collect(b->pickups,a[0],a[1],a[2],a[3]);return;}break;
 case SNIPER_NET_PICKUP_SPAWN:
  if(b->pickups&&b->allocate_pickup){sniper_pickup_create(b->pickups,a[0],a[1],a[2],a[3],b->allocate_pickup,b->user);return;}break;
 case SNIPER_NET_TEAM:
  if(b->team&&r->local&&valid(a[0])){
   SniperPlayerTeam*t=b->team;t->players=r->players;t->ordered=r->ordered;t->local=r->local;t->count=r->count;t->selected=r->selected;t->dirty=r->dirty;
   sniper_player_team(t,a[0],a[1]);r->selected=t->selected;r->dirty=t->dirty;return;
  }break;
 case SNIPER_NET_ATTACH:
  if(b->local_id&&b->attachment_hud&&valid(a[0])&&(a[1]==-1||valid(a[1]))){sniper_attachment(r->players,*b->local_id,b->attachment_hud,a[0],a[1]);return;}break;
 default:break;
 }
 extension(b,e);
}
