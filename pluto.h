#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <iio.h>  // libiio-dev must be installed
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include "kmlib/km_helper.h"
#include "kmlib/kmfifo.h"
#include "kmlib/kmtimer.h"
#include "udp/udp.h"

// ============ INITIALISATION START ================================
// here are the values which should be set before using this program

// UDP settings
#define UDP_TXSAMPLEPORT  40809			// samples from Pluto are send as UDP frames to this port
#define UDP_RXSAMPLEPORT  40809			// samples received on this port are sent to pluto for transmission

// the pluto runs with its selected sample rate
// these two settings reduce the load on the UDP interface, not the pluto itself
#define RX_DECIMATION 32				// Decimation/Interpolation must be power of 2 !!!
#define TX_INTERPOLATION 32

// Pluto settings
#define SAMPRATE	2.4					// RX and TX sample rate in MHz
#define RX_FREQ		145					// RX base frequency in MHz
#define TX_FREQ		435					// TX base frequency in MHz
#define RX_BW		2					// RX bandwidth in MHz
#define TX_BW		0.1					// TX bandwidth in MHz

// ============ INITIALISATION END ==================================

#define UDPFRAG 32768			// size of an UDP fragment
#define BUFSIZE (1024*1024)		// size of rx and tx data buffers

#define MHZ(x) ((long long)(x*1000000.0 + .5))
#define GHZ(x) ((long long)(x*1000000000.0 + .5))

enum iodev { 
	RX = 0, 
	TX 
};

typedef struct _stream_cfg_ {
	long long bw_hz; // Analog bandwidth in Hz
	long long fs_hz; // Baseband sample rate in Hz
	long long lo_hz; // Local oscillator frequency in Hz
	const char* rfport; // Port name
} stream_cfg;


int pluto_get_IP(char *url_IP);
int pluto_get_USB();
bool get_ad9361_stream_ch(__notused struct iio_context *ctx, enum iodev d, struct iio_device *dev, int chid, struct iio_channel **chn);
bool cfg_ad9361_streaming_ch(struct iio_context *ctx, stream_cfg *cfg, enum iodev type, int chid);
bool get_ad9361_stream_dev(struct iio_context *ctx, enum iodev d, struct iio_device **dev);
void pluto_setup();
void runloop();

extern char pluto_context_name[50];
extern char pluto_ip[20];
extern struct iio_context *ctx;
extern struct iio_device *tx;
extern struct iio_device *rx;
extern struct iio_channel *rx0_i;
extern struct iio_channel *rx0_q;
extern struct iio_channel *tx0_i;
extern struct iio_channel *tx0_q;
extern struct iio_buffer  *rxbuf;
extern struct iio_buffer  *txbuf;
extern stream_cfg rxcfg;
extern stream_cfg txcfg;
extern char *myIP;
extern int udpRXfifo;

