#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <iio.h>  // libiio-dev must be installed
#include <arpa/inet.h>

int pluto_find();
int setup_pluto();
int pluto_create_RXthread();
int pluto_create_TXthread();

extern char *pluto_ip;