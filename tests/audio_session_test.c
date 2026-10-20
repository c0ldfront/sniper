/* Include production session to exercise its private recovered callback wiring. */
#include "../src/offline_session.c"
#include <stdio.h>
#define CHECK(x) do{if(!(x)){printf("FAIL line%d\n",__LINE__);return 1;}}while(0)
typedef struct {int n,resource[16],stop[16];SniperAudioRequest requests[16];} Capture;
static void capture(void *user,int32_t resource,const SniperAudioRequest *r){Capture *c=user;c->resource[c->n]=resource;c->requests[c->n++]=*r;}
static void stopped(void *user,int32_t resource){Capture *c=user;c->stop[c->n]=1;c->resource[c->n++]=resource;}
int main(void){
 SniperOfflineSession *s=calloc(1,sizeof *s);CHECK(s);SniperShipObject local={0};Capture c={0};
 s->ships.players[0]=&local;s->frame.width=640;s->host.user=&c;s->host.audio=capture;s->host.audio_stop=stopped;
 s->ships.config.word[(0x4282a5-0x4281c9)/4]=100;s->ships.random_state=87;
 local.word[0x48/4]=1000000;local.word[0x4c/4]=2000000;
 uint32_t expected=87;int32_t random=(int32_t)sniper_random(&expected);
 sound(s,162,1100,2200);CHECK(s->ships.random_state==expected&&c.n==1&&c.resource[0]==162);
 CHECK(c.requests[0].volume==-800&&c.requests[0].pan==3125&&c.requests[0].pitch==random%200-100);
 s->host.audio=NULL;sniper_random(&expected);sound(s,162,1100,2200);CHECK(s->ships.random_state==expected&&c.n==1);
 sound(s,162,1900,2200);sound(s,162,-1,-1);CHECK(s->ships.random_state==expected);
 s->host.audio=capture;prize_sound(s,148);CHECK(s->ships.random_state==expected&&c.n==2&&c.resource[1]==148);
 CHECK(!c.requests[1].volume&&!c.requests[1].pan&&!c.requests[1].loop&&!c.requests[1].pitch);
 impact(s,1000,2000);CHECK(c.n==3&&c.resource[2]==134);
 SniperFlightEffects effects={0};effects.sound_count=3;effects.sound_slot[0]=1;effects.sound_play[0]=1;effects.sound_slot[1]=2;effects.sound_slot[2]=0;effects.sound_play[2]=1;
 flight_audio(s,&effects);CHECK(c.n==6&&c.resource[3]==127&&c.requests[3].loop==1&&c.resource[4]==169&&c.stop[4]&&c.resource[5]==125);
 engine_stop(s);CHECK(c.n==9&&c.resource[6]==125&&c.resource[7]==127&&c.resource[8]==169&&c.stop[6]&&c.stop[7]&&c.stop[8]);
 free(s);puts("PASS session audio wiring: resource identities,direct/positional RNG,mute-independent RNG,distance gates,engine ordering and stop");return 0;
}
