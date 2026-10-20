#ifndef SNIPER_NETWORK_SOCKET_H
#define SNIPER_NETWORK_SOCKET_H
#include <stdint.h>
#include <stddef.h>
/* Native 401090/401440 TCP bootstrap -> connected UDP endpoint.
 * Isolated socket owner, no global game/window dependencies. */
typedef struct { uintptr_t socket; int started,connected; } SniperNetworkSocket;
int sniper_network_socket_open(SniperNetworkSocket *,const char *hostname,uint16_t tcp_port);
void sniper_network_socket_close(void *);
uint32_t sniper_network_socket_clock(void *);
int32_t sniper_network_socket_send(void *,const uint8_t *,int32_t);
int32_t sniper_network_socket_read(void *,uint8_t *,int32_t,int peek);
#endif
