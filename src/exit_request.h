#ifndef SNIPER_EXIT_REQUEST_H
#define SNIPER_EXIT_REQUEST_H
#include "notices.h"
/*405564..405625: Escape closes a nonzero overlay mode; otherwise requests
 * exit and inserts the original1500-tick notice. Frame update posts shutdown
 * later when local energy is strictly greater than500000. */
void sniper_exit_request(int32_t *mode,int32_t *pending,SniperNotices *);
#endif
