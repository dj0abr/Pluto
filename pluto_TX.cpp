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
* Pluto TX thread
* ===============
* 
*/

#include "pluto.h"

void *txproc(void *pdata);

pthread_t tx_pid = 0;

int pluto_create_TXthread()
{
    int ret = pthread_create(&tx_pid,NULL,txproc, NULL);
    if(ret)
    {
        printf("TX thread NOT started\n");
        return 0;
    }

    return 1;
}

double cnt=0;
double freq = 1500;
int sc = 0;

void *txproc(void *pdata)
{
    int bytesleft = 0;
    uint8_t fdat[UDPFRAG];
    int bidx = 0;

    pthread_detach(pthread_self());

    printf("TX thread started\n");

    iio_buffer *txbuf = iio_device_create_buffer(txdev, PLUTO_TXBUFSIZE, false); // TODO, check best buffer size
	if (!txbuf) printf("Could not create TX buffer\n");

    while(keeprunning && txbuf)
    {
        // Schedule TX buffer
		ssize_t nbytes_tx = iio_buffer_push(txbuf);
		if (nbytes_tx < 0) 
        { 
            printf("Pluto TX: Error pushing %d bytes into txbuf\n", (int) nbytes_tx);
        }

        // Get pointers to TX buf and write IQ to TX buf port 0

		ptrdiff_t  p_inc = iio_buffer_step(txbuf);
        char *p_start = (char *)iio_buffer_first(txbuf, tx0_i);
		char *p_end = (char *)iio_buffer_end(txbuf);
		for (char *p_dat = p_start; p_dat < p_end; p_dat += p_inc) 
        {
            // samp rate is 3.6MS/s

            /* create a sine wave
            cnt += (freq*10000/11111) * 2 / 1000000;
            double i = cos(cnt);
            double q = sin(cnt);
            double ival = (int16_t)(i * 30000);
            double qval = (int16_t)(q * 30000);
			((int16_t*)p_dat)[0] = ival; // Real (I)
			((int16_t*)p_dat)[1] = qval; // Imag (Q)
            */

            if(bytesleft == 0)
            {
                // read new data from fifo
                while(1)
                {
                    int lread = read_fifo(fifoid, fdat, UDPFRAG);
                    if(!keeprunning) 
                    {
                        printf("TX thread stopped\n");
                        pthread_exit(NULL);
                    }
                    if(lread)
                    {
                        bytesleft = lread; //new bytes in fdat
                        bidx = 0;
                        break;
                    }
                    usleep(10000);
                }
            }

            int16_t *ps = (int16_t *)(&fdat[bidx]);

			((int16_t*)p_dat)[0] = *ps++; // Real (I)
			((int16_t*)p_dat)[1] = *ps; // Imag (Q)
            bidx += 4;
            bytesleft -= 4;
		}

        usleep(10000);
    }

    printf("TX thread stopped\n");
    pthread_exit(NULL);
}

