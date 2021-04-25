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

char *myIP;
char pluto_ip[20] = {""}; // enter IP address if pluto is connected via ethernet
int udpsock = 0;
int udpRXfifo = 0;

void udprxfunc(uint8_t *buffer, int len, struct sockaddr_in* fromsock)
{
	//printf("got %d\n",len);
	write_fifo(udpRXfifo, buffer,len);
}

void close_program()
{
    printf("got Ctrl-C\n");
    keeprunning = 0;
}

int main ()
{
	int res = 0;

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
    /*int res = pluto_get_IP(pluto_ip);
    if(res) res = pluto_setup();

    if(!res) */
    {
        //printf("Pluto not found on ETH\n");
        // Pluto not found on Ethernet, try with USB
        res = pluto_get_USB();
        //if(res) res = pluto_setup();
        if(!res)
        {
            printf("Pluto not found, exit program\n");
            exit(0);
        }
    }

	udpRXfifo = create_fifo(4*(4 * BUFSIZE/UDPFRAG), UDPFRAG);
	UdpRxInit(&udpsock,UDP_RXSAMPLEPORT,udprxfunc,&keeprunning);

	// RX stream config
	rxcfg.bw_hz = MHZ(RX_BW);   	// rx rf bandwidth
	rxcfg.fs_hz = MHZ(SAMPRATE);   	// rx sample rate
	rxcfg.lo_hz = MHZ(RX_FREQ); 	// rx rf frequency
	rxcfg.rfport = "A_BALANCED"; 	// port A (select for rf freq.)

	// TX stream config
	txcfg.bw_hz = MHZ(TX_BW); 		// tx rf bandwidth
	txcfg.fs_hz = MHZ(SAMPRATE);   	// tx sample rate (must be same as RX samp rate)
	txcfg.lo_hz = MHZ(TX_FREQ); 	// tx rf frequency
	txcfg.rfport = "A"; 			// port A (select for rf freq.)

	pluto_setup();

	printf("* Starting IO streaming (press CTRL+C to cancel)\n");
	while (keeprunning)
	{
		runloop();
	}

	if (rxbuf) iio_buffer_destroy(rxbuf); 
	if (txbuf) iio_buffer_destroy(txbuf); 

	if (rx0_i) iio_channel_disable(rx0_i);
	if (rx0_q) iio_channel_disable(rx0_q);
	if (tx0_i) iio_channel_disable(tx0_i);
	if (tx0_q) iio_channel_disable(tx0_q);

	if (ctx) iio_context_destroy(ctx);
	printf("exit program\n");
	exit(0);

	return 0;
} 
