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
* ==========================================
* find a pluto connected via USB or Ethernet
* ==========================================
* 
*/

#include "pluto.h"

char pluto_context_name[50];

int pluto_find()
{
    // === connect Pluto at a given IP address ===
    
    struct sockaddr_in sa;
    
    int res = inet_pton(AF_INET,pluto_ip,&(sa.sin_addr));
    if(res == 1)
    {
        // we have a valid pluto IP continue using this IP
        sprintf(pluto_context_name,"ip:%s",pluto_ip);
        printf("search PLUTO at IP: <%s>\n",pluto_context_name);
        if(setup_pluto() == 1)
        {
            if(pluto_create_RXthread() == 1)
            {
                if(pluto_create_TXthread() == 1)
                {
                    printf("PLUTO initialized: OK\n");
                    return 1;
                }
            }
        }
        printf("PLUTO not found at IP: %s\n",pluto_context_name);
    }
    
    // === connect Pluto via USB ===
    printf("search PLUTO at USB\n");
    char s[500];
    snprintf(s,499,"iio_info -s 2>/dev/null");
    s[499] = 0;
    FILE *fp = popen(s,"r");
    if(fp)
    {
        while (fgets(s, sizeof(s)-1, fp) != NULL) 
        {
            char *hp = strstr(s,"[usb:");
            if(hp)
            {
                hp += 1;
                char *he = strchr(hp,']');
                if(he)
                {
                    *he = 0;
                    strncpy(pluto_context_name,hp,49);
                    pluto_context_name[49] = 0;
                    printf("PLUTO found: <%s>\n",pluto_context_name);
                    if(setup_pluto() == 1)
                    {
                        if(pluto_create_RXthread() == 1)
                        {
                            if(pluto_create_TXthread() == 1)
                            {
                                printf("PLUTO initialized: OK\n");
                                return 1;
                            }
                        }
                    }
                    printf("PLUTO found, but cannot initialize\n");
                    return 0;
                }
            }
        }
        pclose(fp);
    }
    else
        printf("ERROR: cannot execute iio_info command\n");
    
    printf("no PLUTO found\n");
    return 0;
}