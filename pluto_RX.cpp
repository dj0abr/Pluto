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
* ===============
* Pluto RX thread
* ===============
* 
*/

#include "pluto.h"

void *rxproc(void *pdata);
void send_buffer(uint8_t *pdat, int len);

pthread_t rx_pid = 0;

int pluto_create_RXthread()
{
    int ret = pthread_create(&rx_pid,NULL,rxproc, NULL);
    if(ret)
    {
        printf("RX thread NOT started\n");
        return 0;
    }

    return 1;
}

void *rxproc(void *pdata)
{
    pthread_detach(pthread_self());
    printf("RX thread started\n");

    iio_buffer *rxbuf = iio_device_create_buffer(rxdev, PLUTO_RXBUFSIZE, false);
    if (!rxbuf) printf("Could not create RX buffer\n");

    while(keeprunning && rxbuf)
    {
        // === RX from Pluto ===
        // read new data into rx buffer
		int nbytes_rx = (int)iio_buffer_refill(rxbuf);
        if (nbytes_rx > 0) 
        {
            // nbytes_rx have been filled into rxbuf
            // each sample is 4 Bytes long: 2xI and 2xQ (signed short 16 bit values)
            // send these bytes via UDP, fragmented into chunks of max UDPFRAG
            uint8_t *pdat = (uint8_t *)iio_buffer_first(rxbuf, rx0_i);
            send_buffer(pdat, (int)nbytes_rx);
        }
        else
            usleep(10000);
    }

    if (rxbuf) iio_buffer_destroy(rxbuf);
    printf("RX thread stopped\n");
    pthread_exit(NULL);
}

void send_buffer(uint8_t *pdat, int len)
{
    // pdat ... start of the buffer
    // each sample has 4 bytes
    // step through the buffer with a step size of (RX_DECIMATION * 4)

    // stepsize through the samples
    int step = (RX_DECIMATION * 4);
    int ubuflen = len;
    uint8_t *ubuf = pdat;

    if(RX_DECIMATION > 1)
    {
        // create the new buffer to send
        ubuflen = len / RX_DECIMATION;
        ubuf = (uint8_t *)malloc(ubuflen);

        // fill this buffer with the samples
        uint8_t *d = ubuf;
        for(int i=0; i<len; i+=step)
        {
            // copy one sample (4 bytes)
            *d++ = pdat[i];
            *d++ = pdat[i+1];
            *d++ = pdat[i+2];
            *d++ = pdat[i+3];

            // security check
            if(d > (ubuf + ubuflen))
            {
                printf("ATTENTION: ubuf overrun. Stop program\n");
                exit(0);
            }
        }
    }

    // here we have the received samples, decimated by RX_DECIMATION
    // in ubuf with length ubuflen

    // send these bytes via UDP, fragmented into chunks of max UDPFRAG
    int bytesleft = ubuflen;
    uint8_t *p_dat = ubuf;
    uint8_t *p_end = ubuf + ubuflen;
    while(1)
    {
        if(bytesleft <= UDPFRAG)
        {
            //printf("last %d\n",bytesleft);
            // only a single udp fragment left, send all
            sendUDP(myIP, UDP_SAMPLEPORT, p_dat, bytesleft);
            break;  // finished
        }

        //printf("left %d\n",bytesleft);

        // send a complete udp fragment
        sendUDP(myIP, UDP_SAMPLEPORT, p_dat, UDPFRAG);
        bytesleft -= UDPFRAG;
        p_dat += UDPFRAG;
        if(bytesleft == 0) break;

        // security check for overrun
        if(p_dat > p_end)
        {
            printf("ATTENTION: overrun in UDP send routine. Stop program\n");
            exit(0);
        }
    }

    if(RX_DECIMATION > 1)
        free(ubuf);
}