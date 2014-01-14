/* 
 * Copyright (C) 2013 Martynas Bagdonas
 * 
 * http://martynas.bagdonas.net/
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */

struct pollfd {
 SOCKET    fd;       /* file descriptor */
 short  events;   /* events to look for */
 short  revents;  /* events returned */
};


 #ifndef POLLIN
#define POLLIN 1
#endif
#ifndef POLLOUT
#define POLLOUT 2
#endif
#ifndef POLLPRI
#define POLLPRI 4
#endif
#ifndef POLLERR
#define POLLERR 8
#endif
#ifndef POLLHUP
#define POLLHUP 16
#endif
#ifndef POLLNVAL
#define POLLNVAL 32
#endif

int mypoll(struct pollfd *fds, unsigned int nfds, int timeout);

int socksend(SOCKET sock, unsigned char * buf, int bufsize, int to);

int socksendto(SOCKET sock, struct sockaddr_in * sin, unsigned char * buf, int bufsize, int to);

uint32_t timespec_to_ms(const struct timespec *tp);
int64_t timespec_diff_ms(struct timespec *timeA_p, struct timespec *timeB_p);