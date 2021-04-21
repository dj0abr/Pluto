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

void *txproc(void *pdata)
{
    pthread_detach(pthread_self());

    printf("TX thread started\n");
    while(keeprunning)
    {

    }

    printf("TX thread stopped\n");
    pthread_exit(NULL);
}
