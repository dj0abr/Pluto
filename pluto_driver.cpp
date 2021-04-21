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
* =======================================
* various subroutines to access the pluto
* =======================================
* 
*/

#include "pluto.h"

int get_ad9361_streaming_channel(struct iio_context *ctx, int rxtx, struct iio_device *dev, int iq, struct iio_channel **chn)
{
    char s[50];
    sprintf(s,"voltage%d",iq);

	*chn = iio_device_find_channel(dev, s, rxtx);
	if (!*chn)
    {
        sprintf(s,"altvoltage%d",iq);
		*chn = iio_device_find_channel(dev, s, rxtx);
    }

	return *chn==NULL?0:1;
}
