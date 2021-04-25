/*
* super simple UDP handler
* ========================
* Author: DJ0ABR
*
*   (c) DJ0ABR
*   www.dj0abr.de
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
* 
*/

#include "../pluto.h"

void* threadfunction(void* param);

int rxcfg_idx = 0;
int sock;
int port = UDP_SAMPLEPORT;

// start UDP reception
// sock ... pointer to a socket (just a pointer to an int)
// port ... own port, messages only to this port are received
// rxfunc ... pointer to a callback function, will be called for received data
// keeprunning ... pointer to an int. If it is set to 0, the function exits
void UdpRxInit()
{
    // bind port
    struct sockaddr_in sin;

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == -1){
		printf("Failed to create Socket\n");
		exit(0);
	}
	
	char enable = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    // set non-blocking
    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 10;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

	memset(&sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
    sin.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) != 0)
	{
		printf("Failed to bind socket, port:%d\n",port);
        close(sock);
		exit(0);
	}

    printf("port %d sucessfully bound\n", port);
}

int udp_receiver(uint8_t *(*p))
{
	int recvlen;
    const int maxUDPpacketsize = 100000;
	static char rxbuf[maxUDPpacketsize];
	struct sockaddr_in fromSock;
	socklen_t fromlen = sizeof(struct sockaddr_in);

    recvlen = recvfrom(sock, rxbuf, maxUDPpacketsize, 0, (struct sockaddr *)&fromSock, &fromlen);
    if (recvlen > 0)
    {
        // data received: rxbuf with recvlen
        *p = (uint8_t *)rxbuf;
        return recvlen;
    }
    if (recvlen < 0)
    {
        printf("UDPrx: %d\n");
    }
    return 0;
}

// send UDP message
void sendUDP(char *destIP, int destPort, uint8_t *pdata, int len)
{
    int sockfd; 
    struct sockaddr_in     servaddr; 
  
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        printf("sendUDP: socket creation failed\n"); 
        exit(0); 
    } 
    memset(&servaddr, 0, sizeof(servaddr)); 
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(destPort); 
    printf("Send to <%s><%d> Len:%d\n",destIP,destPort,len);
    servaddr.sin_addr.s_addr=inet_addr(destIP);
    ssize_t sent = sendto(sockfd, (char *)pdata, len, 0, (const struct sockaddr *) &servaddr, sizeof(servaddr)); 
    if(sent != (ssize_t)len)
    {
        printf("UDP send error: %d\n",(int)sent);
    }
    usleep(100);
    close(sockfd);
}

