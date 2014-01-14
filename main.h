/* 
 * Copyright (C) 2013 Martynas Bagdonas
 * 
 * http://martynas.bagdonas.net/
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */

#ifndef MAIN_H
#define MAIN_H

typedef struct options {
   int daemon;
   int debug_log;
   int console;
   char log_file[256];
   unsigned short server_port;
   char db_host[256];
   unsigned short db_port;
   char db_name[128];
   char db_user[64];
   char db_password[128];
   char db_task_table[128];
   char db_user_table[128];
   char db_userid_field[128];
   char db_session_field[128];
   char db_connected_field[1280];
} options_t;

extern options_t opt;

#endif	/* MAIN_H */

