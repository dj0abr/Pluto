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
* Your application itself has nothing to do with the Pluto, it just streams
* the data via UDP
* =====================================================================
* 
*/

#include "pluto.h"

char *myIP;
char destIP[20] = UDP_IPADDRESS;
char pluto_ip[20] = PLUTO_IPADDRESS;
int udpsock = 0;
int udpRXfifo = 0;

void udprxfunc(uint8_t *buffer, int len, struct sockaddr_in* fromsock)
{
	//printf("got %d\n",len);
	write_fifo(udpRXfifo, buffer,len);
}

void close_program()
{
    printf("\nCtrl-C pressed\n");
    keeprunning = 0;
}

int main ()
{
	int res = 0;

    install_signal_handler(close_program);

    // read own IP address
	if(strlen(destIP) >= 7)
	{
		myIP = destIP;
	}
	else
	{
		// use local IP
		myIP = ownIP();
		if(myIP == NULL || strlen(myIP) < 7)
		{
			printf("cannot get own IP adress. Network failure. Exit program\n");
			exit(0);
		}
	}
    printf("application IP adress: <%s>\n",myIP);

    // find a pluto connected via USB or Ethernet
    res = pluto_get_IP(pluto_ip);
    if(!res) 
    {
        // Pluto not found on Ethernet, try with USB
        res = pluto_get_USB();
        if(!res)
        {
            printf("Pluto not found, exit program\n");
            exit(0);
        }
    }
	printf("Pluto IP/USB adress: <%s>\n",pluto_context_name);

	udpRXfifo = create_fifo(4*(4 * BUFSIZE/UDPFRAG), UDPFRAG);
	UdpRxInit(&udpsock,UDP_RXSAMPLEPORT,udprxfunc,&keeprunning);
	printf("Samples App->Pluto will be sent via UDP port UDP_RXSAMPLEPORT: %d\n",UDP_RXSAMPLEPORT);
	printf("Samples Pluto->App will be sent via UDP port UDP_TXSAMPLEPORT: %d\n",UDP_TXSAMPLEPORT);
	printf("Status messages will be sent via UDP to port UDP_STATUSPORT  : %d\n",UDP_STATUSPORT);

	pluto_setup();

	printf("Starting RX/TX streaming (press Ctrl+C to cancel)\n");
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
