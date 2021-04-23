#define _LINUX_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void UdpRxInit(int *sock, int port, void (*rxfunc)(uint8_t *, int, struct sockaddr_in*), int *keeprunning);
void sendUDP(char *destIP, int destPort, uint8_t *pdata, int len);

typedef struct {
    int *sock;
    int port;
    void (*rxfunc)(uint8_t *, int, struct sockaddr_in*);
    int *keeprunning;
} RXCFG;
