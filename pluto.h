#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <iio.h>  // libiio-dev must be installed
#include <arpa/inet.h>
#include <errno.h>

#define PLUTO_RXBUFSIZE 360000  // no of samples per buffer call
#define PLUTO_TXBUFSIZE 2500000  // no of samples per buffer call

#define kHz(n)  (n * 1000)
#define MHz(n)  (n * 1000000)

#define CHKERR(n,s) {if(!n) {printf("%s",s); return 0;}}

/* common RX and TX streaming params */
typedef struct _stream_cfg_ {
	uint32_t lo_hz;     // Local oscillator frequency in Hz
	uint32_t fs_hz;     // Baseband sample rate in Hz
	uint32_t bw_hz;     // Analog banwidth in Hz
	const char* rfport; // Port name
    double outpwr_dBm;  // output power of TX
} stream_cfg;

int pluto_get_IP(char *url_IP);
int pluto_get_USB();
int pluto_setup();
int pluto_create_RXthread();
int pluto_create_TXthread();
int get_ad9361_streaming_channel(struct iio_context *ctx, int rxtx, struct iio_device *dev, int iq, struct iio_channel **chn);
void pluto_close();
int start_timer(int mSec, void(*timer_func_handler)(void));
void stop_timer(int timer);

extern char *pluto_ip;
extern char pluto_context_name[];
extern stream_cfg rxcfg;
extern stream_cfg txcfg;
extern int keeprunning;
extern struct iio_device *rxdev;
extern struct iio_device *txdev;
extern struct iio_channel *rx0_i;
extern struct iio_channel *rx0_q;
extern struct iio_channel *tx0_i;
extern struct iio_channel *tx0_q;
