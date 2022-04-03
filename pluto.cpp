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
char destIP[20] = {UDP_IPADDRESS};
char plutoid[100] = {PLUTO_ID};
int udpsock = 0;
int udpRXfifo = 0;
int crossbandrepeater = 0;

void udprxfunc(uint8_t *buffer, int len, struct sockaddr_in* fromsock)
{
	push_udp_data(buffer,len);
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

	// overwrite IP from info in config file
	char *p = getConfigElement("UDP_IPADDRESS");
	if(p && strlen(p) < 20)
	{
		printf("CONFIG file %s found, read values\n",CONFIGFILE);
		strcpy(myIP,p);
	}
    printf("application IP adress:   %s\n",myIP);

    // find a pluto connected via USB or Ethernet
	// overwrite the pluto_id with infos from a config file
	p = getConfigElement("PLUTO_ID");
	if(p && strlen(p) < 50)
	{
		strcpy(plutoid,p);
	}

	if(*plutoid != 'i')
	{
		// automatically search a pluto connected via USB
		res = pluto_get_USB(plutoid);
        if(!res)
        {
            printf("no Pluto found on USB, exit program\n");
            exit(0);
        }
	}
	else
		strcpy(pluto_context_name,plutoid);

	printf("Pluto IP/USB adress:     %s\n",pluto_context_name);

	p = getConfigElement("CROSSBANDREPEATER");
	if(p && strlen(p) < 10)
	{
		if(*p == '1')
		{
			printf("crossband repeater mode: ON\n");
			crossbandrepeater = 1;
		} 
	}

	udpRXfifo = create_fifo(50, BUFSIZE*4);
	UdpRxInit(&udpsock,UDP_RXSAMPLEPORT,udprxfunc,&keeprunning);
	printf("App->Pluto: UDP Port:    %d\n",UDP_RXSAMPLEPORT);
	printf("Pluto->App: UDP Port:    %d\n",UDP_TXSAMPLEPORT);
	printf("Status messages Port:    %d\n",UDP_STATUSPORT);

	create_interpolator();
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

	destroy_interpolator();
	printf("exit program\n");
	exit(0);

	return 0;
} 
