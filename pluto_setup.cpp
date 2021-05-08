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
* setup pluto for RX and TX
* =========================
* 
*/

#include "pluto.h"

// device context
struct iio_context *ctx   = NULL;

// Streaming devices
struct iio_device *tx;
struct iio_device *rx;

// channels
struct iio_channel *rx0_i = NULL;
struct iio_channel *rx0_q = NULL;
struct iio_channel *tx0_i = NULL;
struct iio_channel *tx0_q = NULL;

// buffers
struct iio_buffer  *rxbuf = NULL;
struct iio_buffer  *txbuf = NULL;

// Stream configurations
stream_cfg rxcfg;
stream_cfg txcfg;

// fifo status timer
int fifostat;

void sendfifostat()
{
	int fs = fifo_usedspace(udpRXfifo);
	uint8_t p[4];
	p[0] = fs >> 24;
	p[1] = fs >> 16;
	p[2] = fs >> 8;
	p[3] = fs & 0xff;

	sendUDP(myIP, UDP_STATUSPORT, p, 4);
}

void pluto_setup()
{
	int ret; 

	fifostat = start_timer(50,sendfifostat);

	// set default configuration from pluto.h

	uint32_t srate = MHZ(SAMPRATE);
	if(srate < 2100000) srate = 3000000;

	// RX stream config
	rxcfg.bw_hz = MHZ(RX_BW);   	// rx rf bandwidth
	rxcfg.fs_hz = srate;   			// rx sample rate
	rxcfg.lo_hz = MHZ(RX_FREQ); 	// rx rf frequency
	rxcfg.rfport = "A_BALANCED"; 	// port A (select for rf freq.)

	// TX stream config
	txcfg.bw_hz = MHZ(TX_BW); 		// tx rf bandwidth
	txcfg.fs_hz = srate;   			// tx sample rate (must be same as RX samp rate)
	txcfg.lo_hz = MHZ(TX_FREQ); 	// tx rf frequency
	txcfg.rfport = "A"; 			// port A (select for rf freq.)
	txcfg.hwgain = TX_GAIN;

	// overwrite with configuration from a config file
	char *p = getConfigElement("TX_FREQ");
	if(p)
	{
		double v = atof(p);
		if(v > 0) txcfg.lo_hz = MHZ(v);
	}

	p = getConfigElement("RX_FREQ");
	if(p)
	{
		double v = atof(p);
		if(v > 0) rxcfg.lo_hz = MHZ(v);
	}

	p = getConfigElement("TX_GAIN");
	if(p)
	{
		double v = atof(p);
		txcfg.hwgain = v;
	}

	printf("TX frequency: %lld Hz\n",txcfg.lo_hz);
	printf("RX frequency: %lld Hz\n",rxcfg.lo_hz);
	printf("TX Gain     : %f dBm\n",txcfg.hwgain);

	// Initialize Pluto

	ctx = iio_create_context_from_uri(pluto_context_name);
	if(!ctx) {printf("Cannot find a pluto at: %s\n", pluto_context_name); exit(0);}

	int devanz = iio_context_get_devices_count(ctx);
    if(devanz <= 0) 
    {
        printf("NO Pluto devices found\n");
		exit(0);
    }

	ret = get_ad9361_stream_dev(ctx, TX, &tx);
	if(!ret) {printf("TX streaming device not found. Pluto error\n"); exit(0);}
	ret = get_ad9361_stream_dev(ctx, RX, &rx);
	if(!ret) {printf("TX streaming device not found. Pluto error\n"); exit(0);}

	cfg_ad9361_streaming_ch(ctx, &rxcfg, RX, 0);
	cfg_ad9361_streaming_ch(ctx, &txcfg, TX, 0);

	get_ad9361_stream_ch(ctx, RX, rx, 0, &rx0_i);
	get_ad9361_stream_ch(ctx, RX, rx, 1, &rx0_q);
	get_ad9361_stream_ch(ctx, TX, tx, 0, &tx0_i);
	get_ad9361_stream_ch(ctx, TX, tx, 1, &tx0_q);

	if(MHZ(SAMPRATE) < 2100000)
	{
		iio_device *dev9 = iio_context_find_device(ctx, "ad9361-phy");
		ad9361_set_bb_rate(dev9,MHZ(SAMPRATE));
	}

	iio_channel_enable(rx0_i);
	iio_channel_enable(rx0_q);
	iio_channel_enable(tx0_i);
	iio_channel_enable(tx0_q);

	rxbuf = iio_device_create_buffer(rx, RXBUFSIZE, false);
	if (!rxbuf) {
		perror("Could not create RX buffer");
	}
	txbuf = iio_device_create_buffer(tx, BUFSIZE, false);
	if (!txbuf) {
		perror("Could not create TX buffer");
	}
}