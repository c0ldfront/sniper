#define _POSIX_C_SOURCE 200809L
#include "../src/offline_session.c"
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
static int online_received;
static int32_t transport_send(void*u,const uint8_t*p,int32_t n){(void)u;(void)p;return n;}
static void online_receive(void*u,const SniperGameEvent*e){(void)u;assert(e->kind==SNIPER_NET_DATA&&e->args[0]==0);online_received++;}
static int received;static const uint8_t*captured;static size_t captured_size;
static uint32_t test_clock(void*u){(void)u;return 1000;}
static int32_t test_key(void*u,int32_t vk){(void)u;return vk==16?0x8000:0;}
static void voice_receive(void*u,int32_t sender,const uint8_t*p,size_t n){(void)u;assert(sender==0);received++;captured=p;captured_size=n;}
static void write_record(const char*name,const char*text){FILE*f=fopen(name,"wb");assert(f);assert(fwrite(text,1,strlen(text),f)==strlen(text));fclose(f);}
int main(void){
 char directory[]="/tmp/sniper87-voice-XXXXXX";assert(mkdtemp(directory));assert(!chdir(directory));
 write_record("record0.wav","first snapshot");write_record("record1.wav","second snapshot");
 SniperSessionHost host={0};host.clock_ms=test_clock;host.key_state=test_key;host.key_state_event=test_key;
 SniperOfflineSession*s=sniper_session_prepare(&host,640,480,87);assert(s&&s->voice_recordings.data[0]&&s->voice_recordings.data[1]);
 s->voice_receive=voice_receive;SniperShipObject*local=calloc(1,sizeof *local);assert(local);s->ordered[0]=s->ships.players[0]=local;s->ships.player_count=1;local->word[0x70/4]=1;local->word[0x5c/4]=-1;
 sniper_session_bind_local(s,local,0);s->ships.config.word[(0x4282e1-0x4281c9)/4]=1;
 uint8_t*original=s->voice_recordings.data[0];int length=s->voice_recordings.length[0];write_record("record0.wav","changed after game entry");
 s->lifecycle.key=0x7a;s->lifecycle.repeat=0x40000000;input(s,&s->binding);assert(received==1&&captured==original&&captured_size==(size_t)length&&!s->lifecycle.key&&!s->lifecycle.repeat);
 s->lifecycle.key=0x7b;input(s,&s->binding);assert(received==2&&captured==s->voice_recordings.data[1]);
 s->ships.config.word[(0x4282e1-0x4281c9)/4]=0;s->lifecycle.key=0x7b;input(s,&s->binding);assert(received==2);
 s->ships.config.word[(0x4282e1-0x4281c9)/4]=1;local->word[0x70/4]=0;s->lifecycle.key=0x7b;input(s,&s->binding);assert(received==2);assert(!strcmp(s->ships.attachment_hud.text[0],"Player is not receiving audio messages."));
 local->word[0x70/4]=1;
 SniperNetworkClient client;SniperNetworkClientHost nh={0};nh.transport.clock_ms=test_clock;nh.transport.send=transport_send;nh.event=online_receive;sniper_network_client_init(&client,&nh);client.game.player_id=0;
 SniperOnlineSession online={0};online.client=&client;online.gameplay.combat=&s->combat;online.gameplay.client=&client;online.gameplay.local_id=0;s->online=&online;
 client.transport.sender.large_count=1;s->lifecycle.key=0x7a;input(s,&s->binding);assert(!online_received&&!client.cached[0]);
 client.transport.sender.large_count=0;s->lifecycle.key=0x7a;input(s,&s->binding);assert(online_received==1&&client.cached[0]&&client.transport.sender.count>0);
 s->lifecycle.key=0x7a;input(s,&s->binding);assert(online_received==2); /* reliable backlog is not the blocking gate */
 s->online=NULL;sniper_network_client_clear(&client);
 sniper_session_destroy(s);assert(!unlink("record0.wav")&&!unlink("record1.wav"));assert(!chdir("/tmp")&&!rmdir(directory));
 puts("PASS voice session snapshots and production input: both Shift slots, retained entry snapshot, offline/online callbacks, large-transfer gate versus reliable backlog, disabled/capability gates and destruction");
}
