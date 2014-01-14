/* 
 * Copyright (C) 2013 Martynas Bagdonas
 * 
 * http://martynas.bagdonas.net/
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "log.h"
#include "utilities.h"
#include "server.h"
#include "main.h"

void *server_thread(void *par) {
   if (opt.debug_log) {
      _log("[Notice] server_thread: Starting...");
   }
   pthread_t th;
   char *c = NULL, *p = NULL, *v = NULL;

   int gsin_len;
   unsigned int i, r, b, max;
   unsigned long mode = 1;
   int error = 0;
   struct pollfd fds;

   SOCKET gsock, ssock;
   struct sockaddr_in gsin, ssin;
   memset(&ssin, 0, sizeof (ssin));
   ssin.sin_family = AF_INET;
   ssin.sin_port = htons(opt.server_port);
   ssin.sin_addr.s_addr = INADDR_ANY;



   if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
      _log("[Error] server_thread: socket()");
      close(ssock);
      return 0;
   }

   int optval = 1;
   setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, (char *) &optval, sizeof (int));

   if (bind(ssock, (struct sockaddr*) & ssin, sizeof (ssin)) == SOCKET_ERROR) {
      _log("[Error] server_thread: bind()");
      close(ssock);
      return 0;
   }

   if (listen(ssock, SOMAXCONN) == SOCKET_ERROR) {
      _log("[Error] server_thread: listen()");
      close(ssock);
      return 0;
   }

   if (ioctl(ssock, FIONBIO, &mode) == SOCKET_ERROR) {
      _log("[Error] server_thread: ioctlsocket()");
      close(ssock);
      return 0;
   }

   if (opt.debug_log) {
      _log("[Notice] server_thread: New connection request");
   }

   fds.fd = ssock;
   fds.events = POLLIN;

   //	CONNECTION con={0};
   for (;;) {
      for (;;) {

         error = mypoll(&fds, 1, 2000);

         if (error >= 1) break;
         if (error == 0) continue;
         if (errno != EAGAIN && errno != EINTR) {
            //sprintf((char *)buf, "poll(): %s/%d", strerror(errno), errno);
            //if(!srv.silent)(*srv.logfunc)(&defparam, buf);
            break;
         }
         continue;
      }

      if (error < 0)
         break;

      gsin_len = sizeof (gsin);

      if ((gsock = accept(ssock, (struct sockaddr*) & gsin, (socklen_t*) & gsin_len)) == INVALID_SOCKET)
         continue;
      else {
         struct sockaddr_in adr;
         int len = sizeof (struct sockaddr);
         getpeername(gsock, (struct sockaddr*) & adr, (socklen_t*) & len);
         unsigned long u = adr.sin_addr.s_addr;

         if (opt.debug_log) {
            _log("[Notice] server_thread: Connection from %d.%d.%d.%d(%u), SOCKET: %u",
                    (u) & 0x000000ff,
                    (u >> 8) & 0x000000ff,
                    (u >> 16) & 0x000000ff,
                    (u >> 24) & 0x000000ff, u, gsock);
         }

         ioctl(gsock, FIONBIO, &mode);
         client_add(u, gsock);
      }
   }
   return 0;
}




