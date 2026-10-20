#include "network_lifecycle.h"
static uint32_t tick(SniperNetworkTransport*t){return t->host.clock_ms?t->host.clock_ms(t->host.user)/10:0;}
int sniper_network_transport_alive(SniperNetworkTransport*t,int initialized){
    if(!t)return 0;
    if((int32_t)tick(t)<(int32_t)t->last_receive_tick)t->last_receive_tick=tick(t);
    return initialized&&t->sender.connected&&tick(t)-t->last_receive_tick<30000;
}
void sniper_network_transport_reset_ping(SniperNetworkTransport*t){if(t){t->sender.ping_min=9999999;t->sender.ping_received=t->sender.ping_sent=0;t->receiver.best_rtt=9999999;t->receiver.replies=0;}}
void sniper_network_transport_shutdown(SniperNetworkTransport*t,void(*wait)(void*),void*user){
    if(!t)return;
    uint32_t start=tick(t);
    while(!t->sender.closing&&t->sender.count>0){if(tick(t)-start>=800)break;size_t n;(void)sniper_network_transport_receive(t,&n);if(wait)wait(user);}
    if(t->sender.connected){const uint8_t bye[2]={0,4};for(int i=0;i<3;i++)if(t->host.send)t->host.send(t->host.user,bye,2);
        start=tick(t);while(tick(t)-start<100){if(wait)wait(user);}
        if(t->host.send)t->host.send(t->host.user,bye,2);
        if(t->host.close)t->host.close(t->host.user);
    }
    sniper_network_transport_clear(t);
}
