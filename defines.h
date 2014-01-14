/* 
 * Copyright (C) 2013 Martynas Bagdonas
 * 
 * http://martynas.bagdonas.net/
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */

#define ENABLE_LOGGING

#define NUM_MYSQL_CONNECTIONS 3
#define UNINITIALIZED_CLIENT_TIMEOUT_S 20 //client should do an authentication until timeout hits

#define SOCKET int
#define SOCKET_ERROR -1
#define INVALID_SOCKET 0