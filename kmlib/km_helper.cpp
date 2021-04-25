/*
* Raspberry PI / Zero AddOn Board specially for Ham Radio Applications
* ====================================================================
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
kmhelper.cpp
===========

useful functions

*
*/

#include "km_helper.h"

int keeprunning = 1;    // set to 0 at program end to exit all processes

// check if it is already running
int isRunning(char *prgname)
{
    int num = 0;
    char s[256];
    sprintf(s,"ps -e | grep %s",prgname);
    
    FILE *fp = popen(s,"r");
    if(fp)
    {
        // gets the output of the system command
        while (fgets(s, sizeof(s)-1, fp) != NULL) 
        {
            if(strstr(s,prgname) && !strstr(s,"grep"))
            {
                if(++num == 2)
                {
                    printf("%s is already running, do not start twice !\n",prgname);
                    pclose(fp);
                    return 1;
                }
            }
        }
        pclose(fp);
    }
    return 0;
}

void (*sigfunc)();

// signal handler
void sighandler(int signum)
{
    //printf("\n\nprogram stopped by signal\n");

    (*sigfunc)();
}

void install_signal_handler(void (*signalfunction)())
{
    sigfunc = signalfunction;
    // signal handler, mainly used if the user presses Ctrl-C
    struct sigaction sigact;
    sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
    sigaction(SIGABRT, &sigact, NULL); // assert() error
    
    //sigaction(SIGSEGV, &sigact, NULL);
    
    // switch off signal 13 (broken pipe)
    // instead handle the return value of the write or send function
    signal(SIGPIPE, SIG_IGN);


}

void showbitstring(char* title, uint8_t* data, int totallen, int anz)
{
    printf("%s. len %d: ", title, totallen);
    for (int i = 0; i < anz; i++)
        printf("%01X ", data[i]);
    printf("\n");
}

void showbytestring(char *title, uint8_t *data, int totallen, int anz)
{
    printf("%s. len % 4d: ",title, totallen);
    for(int i=0; i<anz; i++)
        printf("%02X ",data[i]);
    printf("\n");
}

void showbytestring16(char *title, uint16_t *data, int anz)
{
    printf("%s. len %d: ",title,anz);
    for(int i=0; i<anz; i++)
        printf("%04X ",data[i]);
    printf("\n");
}

void showbytestring32(char* title, uint32_t* data, int anz)
{
    printf("%s. len %d: ", title, anz);
    for (int i = 0; i < anz; i++)
        printf("%08X ", data[i]);
    printf("\n");
}

void showbytestringf(char* title, float* data, int totallen, int anz)
{
    printf("%s. len %d: ", title, totallen);
    for (int i = 0; i < anz; i++)
        printf("%7.4f ", data[i]);
    printf("\n");
}

// get own IP adress
char* ownIP()
{
    static char ip[20] = { 0 };

    struct ifaddrs* ifaddr, * ifa;
    int s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1)
    {
        printf("cannot read own IP address, getifaddrs faield. Check Networking\n");
        return ip;
    }


    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            if (s != 0)
            {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                printf("cannot read own IP address, getnameinfo failed: %s. Check Networking\n", gai_strerror(s));
                break;
            }
            if (strncmp(host, "127", 3) != 0)
            {
                strcpy(ip, host);
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return ip;
}
