/* 
 * Copyright (C) 2013 Martynas Bagdonas
 * 
 * http://martynas.bagdonas.net/
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */

#include <sys/time.h>
#include <string.h>
#include <stdlib.h>


#include "defines.h"
#include "client.h"
#include "main.h"

//
client_t *g_clients = NULL;

client_t *client_new() {
   client_t *t = (client_t*) malloc(sizeof (client_t));
   memset(t, 0, sizeof (client_t));
   return t;
}

client_t *client_get_last() {
   if (g_clients == NULL)
      return NULL;

   client_t *t = g_clients;

   while (t->next) {
      t = t->next;
   }

   return t;
}

client_t *client_get_by_id(char *id, client_t *t) {
   if (t == NULL)
      return NULL;

   while (t) {
      if (!strcmp(t->id, id))
         return t;
      t = t->next;
   }

   return NULL;
}

unsigned long client_get_total() {
   unsigned long total = 0;
   client_t *t = g_clients;

   while (t) {
      total++;
      t = t->next;
   }

   return total;
}

client_t *client_add(unsigned long ip, SOCKET sock) {
   client_t *t, t2;

   if (g_clients == NULL) {
      t = client_new();
      t->ip = ip;
      memset(t->id, 0, sizeof (t->id));
      t->sock = sock;
      t->time_connected = time(NULL);
      t->active = 1;

      g_clients = t;
      return g_clients;
   }

   t = g_clients;

   while (t) {
      if (!t->active) {
         memset(&t2, 0, sizeof (client_t));
         t2.next = t->next;
         t2.prev = t->prev;
         *t = t2;
         t->initialized = 0;
         t->ip = ip;
         t->sock = sock;
         t->time_connected = time(NULL);
         memset(t->rbuf, 0, sizeof (t->rbuf));
         t->rn = 0;
         t->active = 1;
         return t;
      }
      t = t->next;
   }

   t = client_get_last();
   t->next = client_new();

   t->next->ip = ip;
   memset(t->next->id, 0, sizeof (t->next->id));
   t->next->sock = sock;
   t->next->time_connected = time(NULL);
   t->next->active = 1;
   t->next->prev = t;

   return t->next;
}

