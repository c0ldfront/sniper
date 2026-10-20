#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "network_socket.h"
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
typedef SOCKET socket_type;
#define BAD_SOCKET INVALID_SOCKET
#define socket_close closesocket
static int nonblocking(socket_type s){u_long value=1;return ioctlsocket(s,FIONBIO,&value);}
static void pause_poll(void){Sleep(1);}
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
typedef int socket_type;
#define BAD_SOCKET (-1)
#define socket_close close
static int nonblocking(socket_type s){return fcntl(s,F_SETFL,fcntl(s,F_GETFL,0)|O_NONBLOCK);}
static void pause_poll(void){struct timespec t={0,1000000};nanosleep(&t,NULL);}
#endif
uint32_t sniper_network_socket_clock(void *unused){(void)unused;
#ifdef _WIN32
return GetTickCount();
#else
struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))return 0;return (uint32_t)((uint64_t)t.tv_sec*1000+(uint64_t)t.tv_nsec/1000000);
#endif
}
void sniper_network_socket_close(void *user){SniperNetworkSocket *s=user;if(!s)return;if(s->connected)socket_close((socket_type)s->socket);s->socket=(uintptr_t)BAD_SOCKET;s->connected=0;
#ifdef _WIN32
if(s->started)WSACleanup();
#endif
s->started=0;}
int32_t sniper_network_socket_send(void *user,const uint8_t*p,int32_t n){SniperNetworkSocket*s=user;if(!s||!s->connected||n<0)return -1;return (int32_t)send((socket_type)s->socket,(const char*)p,n,0);}
int32_t sniper_network_socket_read(void *user,uint8_t*p,int32_t n,int peek){SniperNetworkSocket*s=user;if(!s||!s->connected||n<0)return -1;return (int32_t)recv((socket_type)s->socket,(char*)p,n,peek?MSG_PEEK:0);}
int sniper_network_socket_open(SniperNetworkSocket*s,const char*hostname,uint16_t port){
    if(!s||!hostname)return 0;
    memset(s,0,sizeof(*s));s->socket=(uintptr_t)BAD_SOCKET;
#ifdef _WIN32
    WSADATA w;if(WSAStartup(0x101,&w))return 0;s->started=1;if(w.wVersion!=0x101){sniper_network_socket_close(s);return 0;}
#else
    s->started=1;
#endif
    socket_type tcp=BAD_SOCKET,udp=BAD_SOCKET;struct sockaddr_in remote,local;memset(&remote,0,sizeof(remote));remote.sin_family=AF_INET;remote.sin_port=htons(port);
    remote.sin_addr.s_addr=inet_addr(hostname);
    if(remote.sin_addr.s_addr==INADDR_NONE){struct hostent*h=gethostbyname(hostname);if(!h||h->h_length!=4)goto fail;memcpy(&remote.sin_addr,h->h_addr_list[0],4);}
    tcp=socket(AF_INET,SOCK_STREAM,0);if(tcp==BAD_SOCKET||connect(tcp,(struct sockaddr*)&remote,sizeof(remote)))goto fail;
    if(nonblocking(tcp))goto fail;
    int option=-1;setsockopt(tcp,IPPROTO_TCP,TCP_NODELAY,(const char*)&option,sizeof(option));option=1024;setsockopt(tcp,SOL_SOCKET,SO_SNDBUF,(const char*)&option,sizeof(option));setsockopt(tcp,SOL_SOCKET,SO_RCVBUF,(const char*)&option,sizeof(option));
    udp=socket(AF_INET,SOCK_DGRAM,0);if(udp==BAD_SOCKET||nonblocking(udp))goto fail;
    option=-1;setsockopt(udp,IPPROTO_TCP,TCP_NODELAY,(const char*)&option,sizeof(option));option=1536;setsockopt(udp,SOL_SOCKET,SO_SNDBUF,(const char*)&option,sizeof(option));option=8192;setsockopt(udp,SOL_SOCKET,SO_RCVBUF,(const char*)&option,sizeof(option));
#ifdef _WIN32
    int address_size=sizeof(local);
#else
    socklen_t address_size=sizeof(local);
#endif
    memset(&local,0,sizeof(local));if(getsockname(tcp,(struct sockaddr*)&local,&address_size))goto fail;local.sin_port=0;
    if(bind(udp,(struct sockaddr*)&local,sizeof(local)))goto fail;
    address_size=sizeof(local);if(getsockname(udp,(struct sockaddr*)&local,&address_size))goto fail;
    if(send(tcp,(const char*)&local.sin_addr,4,0)!=4||send(tcp,(const char*)&local.sin_port,2,0)!=2)goto fail;
    uint8_t reply[512];uint32_t begin=sniper_network_socket_clock(NULL)/10;
    for(;;){int n=(int)recv(tcp,(char*)reply,sizeof(reply),MSG_PEEK);if(n==6){if(recv(tcp,(char*)reply,6,0)!=6)goto fail;memcpy(&remote.sin_addr,reply,4);memcpy(&remote.sin_port,reply+4,2);if(connect(udp,(struct sockaddr*)&remote,sizeof(remote)))goto fail;break;}
        if(sniper_network_socket_clock(NULL)/10-begin>500)goto fail;
        pause_poll();}
    socket_close(tcp);s->socket=(uintptr_t)udp;s->connected=1;return 1;
fail:
    if(tcp!=BAD_SOCKET)socket_close(tcp);
    if(udp!=BAD_SOCKET)socket_close(udp);
    sniper_network_socket_close(s);return 0;
}
