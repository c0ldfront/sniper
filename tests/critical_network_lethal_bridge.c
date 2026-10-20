/* Actual remote projectile collision and local outgoing death notification. */
#include "critical_network_death_bridge.c"
void critical_lethal_init(uint32_t(*clock)(void*),int32_t(*read)(void*,uint8_t*,int32_t,int),int32_t(*send)(void*,const uint8_t*,int32_t),void(*trace)(const int32_t*,const void*),const void*initial,const uint8_t*map,uint32_t offset,int unused){
 critical_death_init(clock,read,send,trace,initial,map,offset,unused);
 memset(session->ships.coverage,0xff,sizeof session->ships.coverage);
}
