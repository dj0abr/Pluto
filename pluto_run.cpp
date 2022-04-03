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
void interpolate (uint8_t *src, uint8_t *dst, int numbytes);
void create_interpolator();
liquid_float_complex getMarker();

firinterp_crcf TX_interpolator = NULL;

nco_crcf tunenco = NULL;    // 100 Hz Marker
nco_crcf upnco = NULL;      // up mixer

int timestamp(char *s, int mode)
{
static unsigned long tstart;
//static unsigned long tlast;

    struct timeval tv;
    gettimeofday(&tv,NULL);
    if(mode == 1)
    {
        // init start time
        tstart = tv.tv_sec * 1000000 + tv.tv_usec;
        //printf("\nSTART: 0 us diff: %ld ms\n",(tstart-tlast)/1000);
        //tlast = tstart;
        return 0;
    }
    unsigned long tact = tv.tv_sec * 1000000 + tv.tv_usec;
    int tdiff = (int)(tact-tstart) / 1000;
    //printf("%s: %ld ms diff: %ld us\n",s,(tact - tstart)/1000,tact-tlast);
    //tlast = tact;

    return tdiff;
}

void runloop()
{
	ssize_t nbytes_rx, nbytes_tx;
	char *p_dat, *p_start;

	// ====== receive samples from pluto ======

    timestamp("",1);

	// Refill RX buffer
	nbytes_rx = iio_buffer_refill(rxbuf);
	if (nbytes_rx < 0) { printf("Error refilling buf %d\n",(int) nbytes_rx); }

    //timestamp("refill",0);

	p_start = (char *)iio_buffer_first(rxbuf, rx0_i);

	// sample buffer begins at p_start with length (BUFSIZE * 4) bytes
    if(crossbandrepeater == 0)
        send_buffer((uint8_t *)p_start, (RXBUFSIZE*4));
        
    //timestamp("send buffer",0);

	// ====== send samples to pluto ======
    // get samples received via UDP
    static uint8_t pidata[BUFSIZE*4];
    int lenfifo = 1;
    if(crossbandrepeater == 0)
    {
        lenfifo = read_fifo(udpRXfifo, pidata, BUFSIZE*4);
    }
    else
    {
        // crossband repeater mode, send whats received
        uint8_t *pr = (uint8_t *)p_start;
        for(int smp=0; smp<(BUFSIZE*4); smp+=2)
        {
            // Plutos's RX is 12 bit
            // so shift left by 4 before sending
            uint16_t v = pr[smp];
            v<<=8;
            v += pr[smp+1];

            pidata[smp] = v >> 8;
            pidata[smp+1] = v &0xff;
        }
    }

    //timestamp("getFifoData",0);
    if(lenfifo)
    {
        p_dat = (char *)iio_buffer_first(txbuf, tx0_i);
        memcpy(p_dat,pidata,BUFSIZE*4);

        //timestamp("pushing",0);
        nbytes_tx = iio_buffer_push(txbuf);
        //timestamp("pushed",0);
	    if (nbytes_tx < 0) { printf("Error pushing buf %d\n", (int) nbytes_tx); }
    }
    else
    {
        //printf ("=== no data in fifo ===\n");
        usleep(100);
    }
  
    /*
    // activate for loop timing check
    static int maxdur = 0;
    int dur = timestamp("loop end",0);
    if(dur > maxdur) maxdur = dur;
    if(dur > 200)
    {
        printf("possible timing problem in pluto loop:\n");
        printf("Pluto Loop duration %d max %d\n",dur,maxdur);
    }*/
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

    // create the new buffer to send
    ubuflen = len / RX_DECIMATION;
    ubuf = (uint8_t *)malloc(ubuflen);

    // fill this buffer with the samples
    uint8_t *d = ubuf;
    for(int i=0; i<len; i+=step)
    {
        // Pluto's RX resolution is 12 bits
        // we are working with 16 bit samples, so a shift-4 is required
        uint16_t smpi = pdat[i+1];
        smpi<<=8;
        smpi += pdat[i];

        uint16_t smpq = pdat[i+3];
        smpq<<=8;
        smpq += pdat[i+2];

        smpi <<= 4;
        smpq <<= 4;

        // copy one sample (4 bytes)
        *d++ = smpi & 0xff;
        *d++ = smpi >> 8;
        *d++ = smpq & 0xff;
        *d++ = smpq >> 8;

        // security check
        if(d > (ubuf + ubuflen))
        {
            printf("ATTENTION: ubuf overrun. Stop program\n");
            exit(0);
        }
    }

    // here we have the received samples, decimated by RX_DECIMATION
    // in ubuf with length ubuflen

    // send these bytes via UDP, fragmented into chunks of max UDPFRAG
    int frags = 0;
    int bytesleft = ubuflen;
    uint8_t *p_dat = ubuf;
    uint8_t *p_end = ubuf + ubuflen;
    char *plutorxip = myIP;
    while(1)
    {
        if(bytesleft <= UDPFRAG)
        {
            //printf("last %d\n",bytesleft);
            // only a single udp fragment left, send all
            sendUDP(plutorxip, UDP_TXSAMPLEPORT, p_dat, bytesleft);
            frags++;
            break;  // finished
        }

        //printf("left %d\n",bytesleft);

        // send a complete udp fragment
        sendUDP(plutorxip, UDP_TXSAMPLEPORT, p_dat, UDPFRAG);
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

    free(ubuf);

    //printf("%d frags sent\n",frags);
}

uint8_t plutorxbuf[BUFSIZE*4];
int plutoidx = 0;
uint8_t rxb[UDPFRAG];
int rxbidx = -1;
liquid_float_complex udpbuf[100000];
int lenfifo = 0;
#define MAXSIGNED32BIT  2000000000

// TX-Interpolator Filter Parameters
// 44100 input rate for 2205 Sym/s = 20
// change for other rates

unsigned int k_SampPerSymb =            24;         // change in TX_INTERPOLATION
unsigned int m_filterDelay_Symbols =    15;         // not too short for good filter
float        beta_excessBW  =           0.2f;      // filter excess bandwidth factor
float        tau_FracSymbOffset   =     -0.2f;      // fractional symbol offset

void create_interpolator()
{
    // TX: Interpolator Filter

    k_SampPerSymb = TX_INTERPOLATION;

    // compute delay
    while (tau_FracSymbOffset < 0) tau_FracSymbOffset += 1.0f;  // ensure positive tau
    float g = k_SampPerSymb*tau_FracSymbOffset;                 // number of samples offset
    int ds=(int)floorf(g);               // additional symbol delay
    float dt = (g - (float)ds);     // fractional sample offset
    // force dt to be in [0.5,0.5]
    if (dt > 0.5f) 
    {                
        dt -= 1.0f;
        ds++;
    }
    
    // calculate filter coeffs
    unsigned int h_len_NumFilterCoeefs = 2 * k_SampPerSymb * m_filterDelay_Symbols + 1;
    float h[50000];
    if (h_len_NumFilterCoeefs >= 50000)
    {
        printf("h in h_len_NumFilterCoeefs too small, need %d\n", h_len_NumFilterCoeefs);
        exit(0);
    }
    liquid_firdes_prototype(    LIQUID_FIRFILT_RRC,
                                k_SampPerSymb,
                                m_filterDelay_Symbols,
                                beta_excessBW,
                                dt,
                                h);
    // create the filter
    TX_interpolator = firinterp_crcf_create(k_SampPerSymb,h,h_len_NumFilterCoeefs);

    // create marker NCO
    // set the marker to 100Hz
    // the signal is shifted to mid:1500Hz in the receiver, so the marker
    // must be on -1400 Hz (minus: assign sin to real and cos to imag)
    float rad_per_sample = ((2.0f * (float)M_PI * 1400)/((float)SAMPRATE * 1e6));
    tunenco = nco_crcf_create(LIQUID_NCO);
    nco_crcf_set_phase(tunenco, 0.0f);
    nco_crcf_set_frequency(tunenco, rad_per_sample);

    // Upmixer
    float RADIANS_PER_SAMPLE   = ((2.0f * (float)M_PI * 10000)/((float)SAMPRATE * 1e6));
    upnco = nco_crcf_create(LIQUID_NCO);
    nco_crcf_set_phase(upnco, 0.0f);
    nco_crcf_set_frequency(upnco, RADIANS_PER_SAMPLE);
}

void destroy_interpolator()
{
    if(TX_interpolator != NULL) firinterp_crcf_destroy(TX_interpolator);
    TX_interpolator = NULL;
    if(tunenco) nco_crcf_destroy(tunenco);
    tunenco = NULL;
    if(upnco) nco_crcf_destroy(upnco);
    upnco = NULL;
}

liquid_float_complex getMarker()
{
    liquid_float_complex m;
    if (tunenco)
    {
        nco_crcf_step(tunenco);
        m.real = nco_crcf_sin(tunenco);
        m.imag = nco_crcf_cos(tunenco);
    }
    else
        printf("tunenco not initialized\n");

    return m;
}

liquid_float_complex sampleToComplex(uint8_t *rxb)
{
    // first convert to signed 32 bit int
    int32_t re = rxb[3];
    re <<= 8;
    re += rxb[2];
    re <<= 8;
    re += rxb[1];
    re <<= 8;
    re += rxb[0];

    int32_t im = rxb[7];
    im <<= 8;
    im += rxb[6];
    im <<= 8;
    im += rxb[5];
    im <<= 8;
    im += rxb[4];

    // now convert to complex number
    liquid_float_complex c;
    c.real = re / (float)MAXSIGNED32BIT;
    c.imag = im / (float)MAXSIGNED32BIT;        
    return c;
}

// push data into plutorxbuf until it is filled with BUFSIZE*4
void push_udp_data(uint8_t *buffer, int len)
{
    for(int i=0; i<len; i+=8)
    {
        liquid_float_complex c = sampleToComplex(buffer+i);

        // interpolate the sample by TX_INTERPOLATION
        liquid_float_complex yout[TX_INTERPOLATION];
        firinterp_crcf_execute(TX_interpolator, c, yout);
        // the interpolation has created TX_INTERPOLATION new complex samples
        
        // fill these samples (with a marker) into plutorxbuf
        for(int ipol=0; ipol<TX_INTERPOLATION; ipol++)
        {
            liquid_float_complex m = getMarker();

            yout[ipol].real += (m.real * 0.07);
            yout[ipol].imag += (m.imag * 0.07);

            yout[ipol].real *= 0.5;
            yout[ipol].imag *= 0.5;

            if (yout[ipol].real > 1.0 || yout[ipol].real < -1.0) printf("real: %f\n", yout[ipol].real);
            if (yout[ipol].imag > 1.0 || yout[ipol].imag < -1.0) printf("imag: %f\n", yout[ipol].imag);

            int16_t pluto_re = (int16_t)(yout[ipol].real * 32767.0);
            int16_t pluto_im = (int16_t)(yout[ipol].imag * 32767.0);

            // and fill it to plutorxbuf
            plutorxbuf[plutoidx++] = pluto_re & 0xff;
            plutorxbuf[plutoidx++] = pluto_re >> 8;
            plutorxbuf[plutoidx++] = pluto_im & 0xff;
            plutorxbuf[plutoidx++] = pluto_im >> 8;

            // check if plutorxbuf is full
            if(plutoidx >= (BUFSIZE*4))
            {
                //timestamp("buf full",0);
                //printf("Buffer full %d %d\n", plutoidx, BUFSIZE*4);
                plutoidx = 0;
                write_fifo(udpRXfifo, plutorxbuf, BUFSIZE*4);
                //timestamp("",1);
            }
        }
    }
}
