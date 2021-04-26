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
* =========================
* runloop for RX and TX
* =========================
* 
*/

#include "pluto.h"

void send_buffer(uint8_t *pdat, int len);
uint8_t *getFifoData(int num);

void runloop()
{
	ssize_t nbytes_rx, nbytes_tx;
	char *p_dat, *p_end, *p_start;
	ptrdiff_t p_inc;

	// ====== handle buffers ======
	// Schedule TX buffer
	nbytes_tx = iio_buffer_push(txbuf);
	if (nbytes_tx < 0) { printf("Error pushing buf %d\n", (int) nbytes_tx); }

	// Refill RX buffer
	nbytes_rx = iio_buffer_refill(rxbuf);
	if (nbytes_rx < 0) { printf("Error refilling buf %d\n",(int) nbytes_rx); }

	// ====== receive samples from pluto ======
	p_start = (char *)iio_buffer_first(rxbuf, rx0_i);
	p_inc = iio_buffer_step(rxbuf);
	p_end = (char *)iio_buffer_end(rxbuf);

/*// TEST entfernern
    int a=0;
    for(int i=0; i<(BUFSIZE*p_inc); i++)
    {
        p_start[i] = a++;
    }
    // ===============
*/


	// sample buffer begins at p_start with length (BUFSIZE*p_inc) bytes
	send_buffer((uint8_t *)p_start, (BUFSIZE*p_inc));

	static int16_t ibuf[BUFSIZE];
	static int16_t qbuf[BUFSIZE];
	int idx = 0;

	// ====== send samples topluto ======
    // get samples received via UDP
	uint8_t *pidata = getFifoData(BUFSIZE*4);
    if(pidata != NULL)
    {
        //showbytestring("RX2:",(uint8_t *)pidata,32,32);
        for(int i = 0; i < BUFSIZE; i++)
        {
            uint16_t v;
            v = pidata[i*4+1];
            v <<= 8;
            v += pidata[i*4];
            ibuf[i] = (int16_t)v;

            v = pidata[i*4+3];
            v <<= 8;
            v += pidata[i*4+2];
            qbuf[i] = (int16_t)v;
        }

        // and put into pluto's tx buffer
        p_inc = iio_buffer_step(txbuf);
        p_end = (char *)iio_buffer_end(txbuf);
        idx = 0;
        for (p_dat = (char *)iio_buffer_first(txbuf, tx0_i); p_dat < p_end; p_dat += p_inc) 
        {
            ((int16_t*)p_dat)[0] = ibuf[idx]<<4;
            ((int16_t*)p_dat)[1] = qbuf[idx]<<4;
            idx++;
        }	
    }
}

// split data into chucks which can be sent via UDP
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
    int frags = 0;
    int bytesleft = ubuflen;
    uint8_t *p_dat = ubuf;
    uint8_t *p_end = ubuf + ubuflen;
    while(1)
    {
        if(bytesleft <= UDPFRAG)
        {
            //printf("last %d\n",bytesleft);
            // only a single udp fragment left, send all
            sendUDP(myIP, UDP_TXSAMPLEPORT, p_dat, bytesleft);
            frags++;
            break;  // finished
        }

        //printf("left %d\n",bytesleft);

        // send a complete udp fragment
        sendUDP(myIP, UDP_TXSAMPLEPORT, p_dat, UDPFRAG);
        frags++;
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

    //printf("%d frags sent\n",frags);
}

uint8_t udprxbuf[BUFSIZE*4];
int uidx = 0;

// read num bytes from UDP-rx fifo
uint8_t *getFifoData(int num)
{
    static uint8_t rbuf[BUFSIZE*4];

    if(num > (int)sizeof(udprxbuf))
    {
        printf("udprxbuf too small %d vs %d\n",num,(int)sizeof(udprxbuf));
    }

    // read from UDP rx fifo until we have the requested number of bytes
    // if there is nothing in the fifo, return with NULL immediately
    //printf("requested %d\n",num);
    while(1)
    {
        uint8_t rxb[UDPFRAG];
        int len = read_fifo(udpRXfifo, rxb, UDPFRAG);
        if(len == 0) 
        {
            uidx = 0;
            return NULL;
        }

        // we got data, copy into udp rx buffer
        if(len > (int)(sizeof(udprxbuf) - uidx))
        {
            printf("fifo data does not fit, don't use fractional relations between UDPFRAG and BUFSIZE*4\n");
            uidx = 0;
            return NULL;
        }

        memcpy(udprxbuf + uidx, rxb, len);
        uidx += len;
        if(uidx >= num/TX_INTERPOLATION)
        {
            //showbytestring("RX1:",(uint8_t *)udprxbuf,32,32);

            int didx = 0;
            for(int i=0; i<uidx; i+=4)
            {
                for(int j=0; j<TX_INTERPOLATION; j++)
                {
                    rbuf[didx++] = udprxbuf[i];
                    rbuf[didx++] = udprxbuf[i+1];
                    rbuf[didx++] = udprxbuf[i+2];
                    rbuf[didx++] = udprxbuf[i+3];
                }
            }

            //showbytestring("RX2:",(uint8_t *)rbuf,32,32);

            uidx = 0;
            return rbuf;
        }
    }
}
