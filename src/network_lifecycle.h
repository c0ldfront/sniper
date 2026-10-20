#ifndef SNIPER_NETWORK_LIFECYCLE_H
#define SNIPER_NETWORK_LIFECYCLE_H
#include "network_transport.h"
/*4013d0. initialized is the caller's WinSock/subsystem-ready flag. */
int sniper_network_transport_alive(SniperNetworkTransport *,int initialized);
void sniper_network_transport_reset_ping(SniperNetworkTransport *);
/*401280 shutdown. Clock must advance independently (real monotonic host) or
 * on calls (deterministic host); wait callback may yield between polls.
 * Discards application packets during the up-to8-second reliability drain.
 * Includes original four close notifications and1-second separation. */
void sniper_network_transport_shutdown(SniperNetworkTransport *,void (*wait)(void *),void *);
#endif
