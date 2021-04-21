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
stream_cfg rxcfg = {
    MHz(435),           // RX frequencxy (LO)
    MHz(3.6),           // RX sample rate
    kHz(100),           // RX Bandwidth
    "A_BALANCED",       // RF port
    0.0                 // out power nut used
};

// TX configuration
stream_cfg txcfg = {
    MHz(435),           // RX frequencxy (LO)
    MHz(3.6),           // RX sample rate
    kHz(100),           // RX Bandwidth
    "A",                // RF port
    0.0                 // out power 0 dBm
};

// looks like TX and RX sampling rate have to be equal. To be checked !

int keeprunning = 1;

int main()
{
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

    // Pluto found, create RX and TX threads
    res = pluto_create_RXthread();
    if(res) pluto_create_TXthread();
    if(!res) 
    {
        printf("connot create threads, exit program\n");
        exit(0);
    }

    sleep(5);

    keeprunning = 0;
    usleep(100000);
    pluto_close();
}
