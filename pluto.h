#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <iio.h>
#include <ad9361.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include "kmlib/km_helper.h"
#include "kmlib/kmfifo.h"
#include "kmlib/kmtimer.h"
#include "udp/udp.h"
#include "liquid.h"

// ============ INITIALISATION START ================================
// here are the values which should be set before using this program

// config file location
// enter directory and name of the configuration file
#define CONFIGFILE		"/home/odroid/ams_config.txt"

// UDP settings
//#define UDP_IPADDRESS	  "192.168.10.2"			// IP address of the application.
#define UDP_IPADDRESS ""							// if empty, the local IP will be used
										
#define UDP_TXSAMPLEPORT  40808			// samples from Pluto are send as UDP frames to this port
#define UDP_RXSAMPLEPORT  40809			// samples received on this port are sent to pluto for transmission
#define UDP_STATUSPORT	  40810			// this program sends status information

// the pluto runs with its selected sample rate
// these two settings reduce the load on the UDP interface, not the pluto itself
#define RX_DECIMATION 1					// Decimation must be power of 2 !!!
#define TX_INTERPOLATION 750			// 7200...750, 6000...900

// Pluto settings
#define SAMPRATE	1.8					// RX and TX sample rate in MHz
#define RX_FREQ		(1129.250)  // RX base frequency in MHz (10489.250 - 390*24)
#define TX_FREQ		(1290.080 + 0.0015)   // TX base frequency in MHz (oder baseband offset)
#define RX_BW		2.0					// RX bandwidth in MHz
#define TX_BW		0.1					// TX bandwidth in MHz
#define TX_GAIN		-10.0		// TX gain in dBm

// Pluto identification
// this ID can be an IP address in the format:   "ip:192.168.1.2"
// or a serial number (shown with iio_info -s) in the format: "1044730a199700191f001800f3647c5341"
// of if left empty, then it searches for a pluto connected via USB
#define PLUTO_ID	""

// ============ INITIALISATION END ==================================

#define UDPFRAG 32768			// max size of an UDP fragment

#define RXBUFSIZE	(180000*1) 
#define BUFSIZE 	(180000*1)

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
	double hwgain;	 // TX gain
	const char* rfport; // Port name
} stream_cfg;


int pluto_get_IP(char *url_IP);
int pluto_get_USB(char *sn);
bool get_ad9361_stream_ch(__notused struct iio_context *ctx, enum iodev d, struct iio_device *dev, int chid, struct iio_channel **chn);
bool cfg_ad9361_streaming_ch(struct iio_context *ctx, stream_cfg *cfg, enum iodev type, int chid);
bool get_ad9361_stream_dev(struct iio_context *ctx, enum iodev d, struct iio_device **dev);
void pluto_setup();
void runloop();
void create_interpolator();
void destroy_interpolator();
void push_udp_data(uint8_t *buffer, int len);

extern char pluto_context_name[50];
extern char pluto_ip[20];
extern struct iio_context *ctx;
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
