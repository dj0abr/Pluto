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

int pluto_getContext();
int pluto_setup_RX();
int pluto_setup_TX();

// Pluto context
static struct iio_context *ctx   = NULL;

// Streaming devices
struct iio_device *rxdev;
struct iio_device *txdev;

// channels
struct iio_channel *rx0_i;
struct iio_channel *rx0_q;
struct iio_channel *tx0_i;
struct iio_channel *tx0_q;

int pluto_setup()
{
    int res = pluto_getContext();
    if(!res) return 0;
    res = pluto_setup_RX();
    if(!res) return 0;
    res = pluto_setup_TX();
    return res;
}

int pluto_getContext()
{
    ctx = iio_create_context_from_uri(pluto_context_name);
    CHKERR(ctx, "FAILED: Acquiring pluto's IIO context\n");

    unsigned int nr_devices = iio_context_get_devices_count(ctx);
    CHKERR(nr_devices,"no pluto devices found\n");

    return 1;
}

int pluto_setup_RX()
{
    int ret; 

    printf("Pluto %s, setup RX Freq=%u Rate=%d BW=%d\n",pluto_context_name,pluto_rxcfg.lo_hz,pluto_rxcfg.fs_hz,pluto_rxcfg.bw_hz);

    // find receiver device
    rxdev = iio_context_find_device(ctx, "cf-ad9361-lpc");
    CHKERR(rxdev, "no RX streaming devices found\n");

    // get physical device
    struct iio_device *physdev =  iio_context_find_device(ctx, "ad9361-phy");
    CHKERR(physdev, "RX: no physical device found\n");

    // find channel
    struct iio_channel *chn = iio_device_find_channel(physdev, "voltage0", false);
    CHKERR(chn, "RX: channel not found\n");

    // configure channel
    ret = iio_channel_attr_write(chn, "rf_port_select", pluto_rxcfg.rfport);
    CHKERR(ret, "RX: set rf_port_select failed\n");
    ret = iio_channel_attr_write_longlong(chn, "rf_bandwidth", pluto_rxcfg.bw_hz);
    CHKERR(ret?0:1, "RX: set rf_bandwidth failed\n");
    ret = iio_channel_attr_write_longlong(chn, "sampling_frequency", pluto_rxcfg.fs_hz);
    CHKERR(ret?0:1, "RX: set sampling_frequency failed\n");

    // get local oscillator channel and set frequency
    struct iio_channel *chnlo = iio_device_find_channel(physdev, "altvoltage0", true);
    CHKERR(chn, "RX: get LO channel failed\n");
    ret = iio_channel_attr_write_longlong(chnlo, "frequency", pluto_rxcfg.lo_hz);
    CHKERR(ret?0:1, "RX: set frequency failed\n");

    // get streaming channels
    ret = get_ad9361_streaming_channel(ctx, 0, rxdev, 0, &rx0_i);
    CHKERR(ret, "RX: get I channel failed\n");
    ret = get_ad9361_streaming_channel(ctx, 0, rxdev, 1, &rx0_q);
    CHKERR(ret, "RX: get Q channel failed\n");

    // enable streaming channels
    iio_channel_enable(rx0_i);
	iio_channel_enable(rx0_q);

    return 1;
}

int pluto_setup_TX()
{
    int ret;

    printf("Pluto %s, setup TX Freq=%u Rate=%d BW=%d\n",pluto_context_name,pluto_txcfg.lo_hz,pluto_txcfg.fs_hz,pluto_txcfg.bw_hz);

    // find transmitter device
    txdev = iio_context_find_device(ctx, "cf-ad9361-dds-core-lpc");
    CHKERR(txdev, "no TX streaming devices found\n");

    // get physical device
    struct iio_device *physdev =  iio_context_find_device(ctx, "ad9361-phy");
    CHKERR(physdev, "TX: no physical device found\n");

    // find channel
    struct iio_channel *chn = iio_device_find_channel(physdev, "voltage0", true);
    CHKERR(chn, "TX: channel not found\n");

    // configure channel
    ret = iio_channel_attr_write(chn, "rf_port_select", pluto_txcfg.rfport);
    CHKERR(ret, "TX: set rf_port_select failed\n");
    ret = iio_channel_attr_write_longlong(chn, "rf_bandwidth", pluto_txcfg.bw_hz);
    CHKERR(ret?0:1, "TX: set rf_bandwidth failed\n");
    ret = iio_channel_attr_write_longlong(chn, "sampling_frequency", pluto_txcfg.fs_hz);
    CHKERR(ret?0:1, "TX: set sampling_frequency failed\n");
    ret = iio_channel_attr_write_double(chn, "hardwaregain", pluto_txcfg.outpwr_dBm - 10);
    CHKERR(ret?0:1, "TX: set hardwaregain failed\n");

    // get local oscillator channel and set frequency ??? was altvoltage0, does it still work with "1" ???
    struct iio_channel *chnlo = iio_device_find_channel(physdev, "altvoltage1", true);
    CHKERR(chn, "TX: get LO channel failed\n");
    ret = iio_channel_attr_write_longlong(chnlo, "frequency", pluto_txcfg.lo_hz);
    CHKERR(ret?0:1, "TX: set frequency failed\n");

    // get streaming channels
    ret = get_ad9361_streaming_channel(ctx, 1, txdev, 0, &tx0_i);
    CHKERR(ret, "TX: get I channel failed\n");
    ret = get_ad9361_streaming_channel(ctx, 1, txdev, 1, &tx0_q);
    CHKERR(ret, "TX: get Q channel failed\n");

    // enable streaming channels
    iio_channel_enable(tx0_i);
	iio_channel_enable(tx0_q);

    return 1;
}

void pluto_close()
{
   	if (rx0_i) iio_channel_disable(rx0_i);
	if (rx0_q) iio_channel_disable(rx0_q);

	if (ctx) iio_context_destroy(ctx);
}
