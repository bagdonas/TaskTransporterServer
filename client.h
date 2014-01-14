/* 
 * File:   client.h
 * Author: Martynas
 *
 * Created on January 6, 2014, 7:04 PM
 */

#ifndef CLIENT_H
/* 
 * Copyright (C) 2013 Martynas Bagdonas
 * 
 * http://martynas.bagdonas.net/
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */

#define	CLIENT_H

typedef struct client {
   char active;
   char initialized;
   char id[32];
   unsigned long ip;
   SOCKET sock;
   unsigned long time_connected;
   unsigned long time_checked;
   unsigned long time_checked2;
   char rbuf[2048]; //receive buffer
   char sbuf[2048]; //send buffer
   unsigned short rn; //received bytes in rbuf number

   struct client *next;
   struct client *prev;
} client_t;


extern client_t *g_clients;

client_t *client_new();
client_t *client_get_last(); 
client_t *client_get_by_id(char *id, client_t *t);
unsigned long client_get_total();
client_t *client_add(unsigned long ip, SOCKET sock);



#endif	/* CLIENT_H */

