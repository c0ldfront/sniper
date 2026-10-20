#ifndef SNIPER_NETWORK_FILE_RECEIVE_H
#define SNIPER_NETWORK_FILE_RECEIVE_H
#include <windows.h>
#include "../src/network_game_receive.h"
/* Own one staged opcode18 file. Paths are basenames in cwd captured at init;
 * server-provided absolute paths, traversal and Win32 devices are rejected. */
typedef struct {
    char directory[MAX_PATH],hostname[136],port[8];
    char name[16],temporary[MAX_PATH],committed[MAX_PATH];
    HANDLE file;
    uint64_t bytes;
    uint32_t crc;
    int active,failed,received;
} SniperNetworkFileReceive;
int sniper_network_file_receive_init(SniperNetworkFileReceive *,const char *hostname,const char *tcp_port);
/*1 handled/accepted,0 unrelated,-1 rejected/I/O failure.
 * OPEN/WRITE never modify final path. CLOSE commits artifact+source sidecar.
 * Caller should suppress native 'File received' chat when this returns-1. */
int sniper_network_file_receive_event(SniperNetworkFileReceive *,const SniperGameEvent *);
/* Aborts unfinished staged receive; leaves committed artifacts/backups intact. */
void sniper_network_file_receive_clear(SniperNetworkFileReceive *);
#endif
