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

pthread_t rx_pid = 0;
int samps = 0;

void timer1s()
{
    printf("samples: %d\n",samps);
    samps=0;
}

int pluto_create_RXthread()
{
    int ret = pthread_create(&rx_pid,NULL,rxproc, NULL);
    if(ret)
    {
        printf("RX thread NOT started\n");
        return 0;
    }

    start_timer(1000, timer1s);

    return 1;
}

void *rxproc(void *pdata)
{
    pthread_detach(pthread_self());
    printf("RX thread started\n");

    iio_buffer *rxbuf = iio_device_create_buffer(rxdev, PLUTO_RXBUFSIZE, false);

    while(keeprunning)
    {
        // === RX from Pluto ===
        // read new data into rx buffer
		ssize_t nbytes_rx = iio_buffer_refill(rxbuf);
		if (nbytes_rx > 0) 
        {    
            //printf("rxed %ld samples\n",nbytes_rx);
            ptrdiff_t p_inc = iio_buffer_step(rxbuf);
            char *p_start = (char *)iio_buffer_first(rxbuf, rx0_i);
            char *p_end = (char *)iio_buffer_end(rxbuf);
            int size = (p_end - p_start) / p_inc;
            int16_t *pibuf = (int16_t *)malloc(size * sizeof(int16_t));
            int16_t *pqbuf = (int16_t *)malloc(size * sizeof(int16_t));
            int16_t *pi = pibuf;
            int16_t *pq = pqbuf;

            for (char *p_dat = p_start; p_dat < p_end; p_dat += p_inc) 
            {
                *pi++ = ((int16_t*)p_dat)[0]; // Real (I)
                *pq++ = ((int16_t*)p_dat)[1]; // Imag (Q)
                samps++;
            }

            // received data are in pibuf and pqbuf
            // do something with this data

            free(pibuf);
            free(pqbuf);
        }
    }

    if (rxbuf) iio_buffer_destroy(rxbuf);
    printf("RX thread stopped\n");
    pthread_exit(NULL);
}
