/*
* Adalm Pluto Driver
* ==================
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
* =====================================================================
* the purpose of this driver is to have a very easy to use plattform
* for receiving and transmitting data via the Adalm-Pluto.
*
* This is a stand alone program. It connects to the pluto and handles
* all RX and TX.
* The final data are transfered to/from the application via an UDP port
* The application has nothing to do with the Pluto, it just streams
* the data via UDP
* =====================================================================
* 
*/

#include "pluto.h"

// === Pluto Definitions ===
// IP Adress (leave empty if connected via USB)
char *pluto_ip = {"192.168.10.99"};

// RX configuration
stream_cfg pluto_rxcfg = {
    MHz(145),           // RX frequencxy (LO)
    MHz(3.6),           // RX sample rate
    kHz(1000),          // RX Bandwidth
    "A_BALANCED",       // RF port
    0.0                 // out power nut used
};

// TX configuration
stream_cfg pluto_txcfg = {
    MHz(435),   // TX frequencxy (LO)
    MHz(3.6),           // TX sample rate
    kHz(100),           // TX Bandwidth
    "A",                // RF port
    8.0                 // out power 0 dBm
};

// looks like TX and RX sampling rate have to be equal. To be checked !

char *myIP = NULL;

#define CROSSBANDREPEATER

#ifdef CROSSBANDREPEATER
int testsock;
int fifoid;
void rxfunc(uint8_t* pdata, int len, struct sockaddr_in* rxsock)
{
    //printf("udprx: %d bytes\n",len);
    write_fifo(fifoid, pdata, len);
}
#endif 

void close_program()
{
    printf("closing threads\n");
    keeprunning = 0;
    usleep(1000000);    // give the threads some time to stop
    pluto_close();
    printf("end program\n");
    exit(0);
}

int main()
{
    install_signal_handler(close_program);

    // read own IP address
    myIP = ownIP();
    if(myIP == NULL || strlen(myIP) < 7)
    {
        printf("cannot get own IP adress. Network failure. Exit program\n");
        exit(0);
    }
    printf("local IP adress: <%s>\n",myIP);

    // find a pluto connected via USB or Ethernet
    int res = pluto_get_IP(pluto_ip);
    if(res) res = pluto_setup();

    if(!res) 
    {
        printf("Pluto not found on ETH\n");
        // Pluto not found on Ethernet, try with USB
        res = pluto_get_USB();
        if(res) res = pluto_setup();
        if(!res)
        {
            printf("Pluto not found, exit program\n");
            exit(0);
        }
    }

    // create a FIFO for the transmitter
    // all data from this fifo will be sent onthe TX frequency
    fifoid = create_fifo(pluto_rxcfg.fs_hz/UDPFRAG+1, UDPFRAG); // space for 1s of samples

    #ifdef CROSSBANDREPEATER
    // TEST ONLY: self reception
    UdpRxInit(&testsock, UDP_SAMPLEPORT, rxfunc, &keeprunning);
    #endif

    // Pluto found, create RX and TX threads
    pluto_create_RXthread();
    if(res) pluto_create_TXthread();
    if(!res) 
    {
        printf("connot create threads, exit program\n");
        exit(0);
    }

    // nothing to do, program can be terminated by Ctl-C
    while(1) sleep(1);
}
