#ifndef SNIPER87_ONLINE_CONNECTION_H
#define SNIPER87_ONLINE_CONNECTION_H
#include "connection_dialog.h"
#include "network_socket.h"
#include "../src/network_client.h"
typedef struct {
    const char *cache_directory; /* caller-owned writable cache; default current directory */
    uint32_t exe_checksum;
    int bypass_exe_checksum; /* native0x800 bypass flag; configuration source unresolved, version still checked */
} Sniper87OnlineOptions;
typedef struct {
    SniperNetworkSocket socket;
    SniperNetworkClient client;
    uint8_t *level_bytes;
    size_t level_size;
    char level_path[MAX_PATH];
    /* Private transfer/controller fields. Keep object address stable. */
    HANDLE download_file;
    HWND owner,download_window;
    HINSTANCE instance;
    int cancelled,write_failed;
    SniperNetworkClient *transfer_client;
    SniperNetworkClientHost previous_host;
} Sniper87OnlineConnection;
/* Opens/authenticates and verifies cached/downloaded map, then arena.lst.
 * Does not join gameplay. Returned object and client pointers stay stable.
 * Caller can replace client.host.event/user before join. NULL on cancel/error.
 * Existing cache files replaced only after successful validated download. */
Sniper87OnlineConnection *sniper87_online_connect(HWND,HINSTANCE,const Sniper87ConnectionSettings *,const Sniper87OnlineOptions *);
int sniper87_network_download(HWND,HINSTANCE,SniperNetworkClient *,const char *path,uint8_t request_opcode);
void sniper87_online_disconnect(Sniper87OnlineConnection *);
#endif
