/* 
 * Copyright (C) 2013 Martynas Bagdonas
 * 
 * http://martynas.bagdonas.net/
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */

#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <mysql/mysql.h>

#include "defines.h"
#include "utilities.h"

int mypoll(struct pollfd *fds, unsigned int nfds, int timeout) {
   fd_set readfd;
   fd_set writefd;
   fd_set oobfd;
   struct timeval tv;
   unsigned i;
   int num;
   SOCKET maxfd = 0;

   tv.tv_sec = timeout / 1000;
   tv.tv_usec = (timeout % 1000)*1000;
   FD_ZERO(&readfd);
   FD_ZERO(&writefd);
   FD_ZERO(&oobfd);
   for (i = 0; i < nfds; i++) {
      if ((fds[i].events & POLLIN))FD_SET(fds[i].fd, &readfd);
      if ((fds[i].events & POLLOUT))FD_SET(fds[i].fd, &writefd);
      if ((fds[i].events & POLLPRI))FD_SET(fds[i].fd, &oobfd);
      fds[i].revents = 0;
      if (fds[i].fd > maxfd) maxfd = fds[i].fd;
   }
   if ((num = select(((int) (maxfd)) + 1, &readfd, &writefd, &oobfd, &tv)) < 1) return num;
   for (i = 0; i < nfds; i++) {
      if (FD_ISSET(fds[i].fd, &readfd)) fds[i].revents |= POLLIN;
      if (FD_ISSET(fds[i].fd, &writefd)) fds[i].revents |= POLLOUT;
      if (FD_ISSET(fds[i].fd, &oobfd)) fds[i].revents |= POLLPRI;
   }
   return num;
}

int socksend(SOCKET sock, unsigned char * buf, int bufsize, int to) {
   int sent = 0;
   int res;
   struct pollfd fds;

   fds.fd = sock;
   fds.events = POLLOUT;
   do {
      res = mypoll(&fds, 1, to * 1000);
      if (res < 0 && (errno == EAGAIN || errno == EINTR)) continue;
      if (res < 1) break;
      res = send(sock, (char*) buf + sent, bufsize - sent, 0);
      if (res < 0) {
         if (errno == EAGAIN || errno == EINTR) continue;
         break;
      }
      sent += res;
   } while (sent < bufsize);
   return sent;
}

int socksendto(SOCKET sock, struct sockaddr_in * sin, unsigned char * buf, int bufsize, int to) {
   int sent = 0;
   int res;
   struct pollfd fds;

   fds.fd = sock;
   do {
      //	if(conf.timetoexit) return 0;
      fds.events = POLLOUT;
      res = mypoll(&fds, 1, to);
      if (res < 0 && (errno == EAGAIN || errno == EINTR)) continue;
      if (res < 1) break;
      res = sendto(sock, (char*) buf + sent, bufsize - sent, 0, (struct sockaddr *) sin, sizeof (struct sockaddr_in));
      if (res < 0) {
         if (errno != EAGAIN) break;
         continue;
      }
      sent += res;
   } while (sent < bufsize);
   return sent;
}

uint32_t timespec_to_ms(const struct timespec *tp) {
   return (uint32_t) tp->tv_sec * 1000 + (uint32_t) tp->tv_nsec / 10000000;
}

int64_t timespec_diff_ms(struct timespec *timeA_p, struct timespec *timeB_p) {
   return (((timeA_p->tv_sec * 1000000000) + timeA_p->tv_nsec) -
           ((timeB_p->tv_sec * 1000000000) + timeB_p->tv_nsec)) / 1000000;
}









