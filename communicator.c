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
#include <mysql/mysql.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "log.h"
#include "utilities.h"
#include "client.h"
#include "main.h"

MYSQL g_dbc[NUM_MYSQL_CONNECTIONS] = {0};

char g_dbc_error;

void mysql_error_report(MYSQL dbc[], int n_dbc) {
   int i, errno;
   for (i = 0; i < n_dbc; i++) {
      if ((errno = mysql_errno(&dbc[i])) != 0) {
         _log("MYSQL_ERROR(connection:%d)[errno:%d] : %s\n", i, errno, mysql_error(&dbc[i]));
      }
   }
}

void mysql_error_report_all() {
   mysql_error_report(g_dbc, NUM_MYSQL_CONNECTIONS);
}

int handle_mysql_error() {
   int i, errno;
   for (i = 0; i < NUM_MYSQL_CONNECTIONS; i++) {
      if ((errno = mysql_errno(&g_dbc[i])) != 0) {
         if (errno == 2006) {
            g_dbc_error = 1;
         }
      }
      mysql_error_report_all();
      return 0;
   }
}

char* get_safe_string(char *unsafe_str, MYSQL dbc) {
   char *safe_str;
   unsigned int unsafe_len;
   unsafe_len = strlen(unsafe_str);
   safe_str = (char*) malloc(unsafe_len * 2 + 1);
   mysql_real_escape_string(&g_dbc[0], safe_str, unsafe_str, unsafe_len);
   return safe_str;
}

int client_initialize(client_t * client) {
   char str_ip[16] = {0}, sql[1024], resp[1024], tmp[128], safe_str[4096];

   //dynamically allocated variables
   char *user_id, *session_hash, *version;

   char *s, *dat[10] = {0}, ch;

   short n = 0;

   MYSQL_RES *res;
   MYSQL_ROW row;


   user_id = NULL;
   session_hash = NULL;
   version = NULL;

   s = client->rbuf;

   while (n < 6) {
      dat[n++] = s;
      if (!(s = strstr(s, ":")))
         break;
      *s = NULL;
      s++;
   }

   if (n < 2)
      goto end;

   user_id = get_safe_string(dat[0], g_dbc[0]);
   session_hash = get_safe_string(dat[1], g_dbc[0]);
   //version = get_safe_string(dat[2], g_dbc[0]);


   sprintf(str_ip, "%d.%d.%d.%d", (client->ip) & 0x000000ff,
           (client->ip >> 8) & 0x000000ff,
           (client->ip >> 16) & 0x000000ff,
           (client->ip >> 24) & 0x000000ff);

   if (opt.debug_log)
      _log("[Notice] client_initialize: Client authentication details have just been got ( ip: %s, user_id: %s, session_hash: %s )",
           str_ip, user_id, session_hash);

   //////////////////////////////////////////////////////////////////////////////

   strncpy(client->id, user_id, sizeof (client->id) - 1);

   snprintf(sql, 1020, "SELECT id FROM %s WHERE %s = '%s' AND %s='%s' LIMIT 1", opt.db_user_table, opt.db_userid_field, user_id, opt.db_session_field, session_hash);
   if (opt.debug_log)
      _log("[Notice] client_initialize: sql: %s", sql);

   if (mysql_query(&g_dbc[0], sql)) {
      handle_mysql_error();
      goto end;
   }

   res = mysql_store_result(&g_dbc[0]);

   if (res && mysql_num_fields(res) == 1 && (row = mysql_fetch_row(res)) != NULL) {
      if (res) {
         mysql_free_result(res);
         res = NULL;
      }
   } else {
      _log("[Error] client_initialize: Authentication failed!");
      goto end;
   }

   if (*opt.db_connected_field != 0) {
      snprintf(sql, 1020, "UPDATE %s SET %s=1 WHERE %s = '%s'", opt.db_user_table, opt.db_connected_field, opt.db_userid_field, user_id);
      if (opt.debug_log)
         _log("[Notice] client_initialize: sql: %s", sql);

      if (mysql_query(&g_dbc[0], sql)) {
         handle_mysql_error();
         goto end;
      }
   }

   client->initialized = 1;

end:
   if (user_id)
      free(user_id);

   if (session_hash)
      free(session_hash);

   if (version)
      free(version);

   return 1;
}

int client_operation(client_t *client, char operation) {
   MYSQL_RES *res;
   MYSQL_ROW row;
   char sql[4096];
   unsigned long time_current;

   //
   char *rbuf;

   rbuf = NULL;

   time_current = time(NULL);

   if (!client->initialized)
      return 0;

   if (operation == 1) {
      if (*opt.db_connected_field != 0) {
         snprintf(sql, sizeof (sql) - 1, "UPDATE %s SET %s=0 WHERE $s = '%s'", opt.db_user_table, opt.db_connected_field, opt.db_userid_field, client->id);
         if (mysql_query(&g_dbc[0], sql)) {
            return handle_mysql_error();
         }
      }
   } else if (operation == 2) {

      rbuf = get_safe_string(client->rbuf, g_dbc[0]);

      snprintf(sql, sizeof (sql) - 1, "SELECT id FROM %s WHERE direction='s' AND status=5 LIMIT 1", opt.db_task_table);
      if (opt.debug_log)
         _log("[Notice] client_operation: sql: %s", sql);

      if (mysql_query(&g_dbc[0], sql)) {
         return handle_mysql_error();
      }

      res = mysql_use_result(&g_dbc[0]);

      if (res && mysql_num_fields(res) == 1 && (row = mysql_fetch_row(res)) != NULL) {
         snprintf(
                 sql, sizeof (sql) - 1,
                 "UPDATE %s SET status=1, user_id='%s', direction='s', create_t=%u, data='%s' WHERE id='%s'",
                 opt.db_task_table, client->id, time(NULL), rbuf, row[0]);
         if (opt.debug_log)
            _log("[Notice] client_operation: sql: %s", sql);

         if (mysql_query(&g_dbc[1], sql)) {
            return handle_mysql_error();
         }

         mysql_free_result(res);
         res = NULL;
      } else {
         snprintf(sql, sizeof (sql) - 1,
                 "INSERT INTO %s (id, status, user_id, direction, create_t, data) VALUES ('',1,'%s','s',%u,'%s')",
                 opt.db_task_table, client->id, time(NULL), rbuf);
         if (opt.debug_log)
            _log("[Notice] client_operation: sql: %s", sql);

         if (mysql_query(&g_dbc[0], sql)) {
            return handle_mysql_error();
         }
      }
   }

   if (rbuf)
      free(rbuf);

   return 1;
}

int data_refresh() {
   client_t *client;
   char sql[1024], resp[1024], tmp[128];

   MYSQL_RES *res, *res2;
   MYSQL_ROW row, row2;


   snprintf(sql, 1020, "SELECT id, user_id, data FROM %s WHERE direction='c' AND status=1", opt.db_task_table); //Some LIMIT required
   if (opt.debug_log)
      _log("[Notice] data_refresh: sql: %s", sql);

   if (mysql_query(&g_dbc[0], sql)) {
      return handle_mysql_error();
   }
   res = mysql_use_result(&g_dbc[0]);

   while (res && mysql_num_fields(res) == 3 && (row = mysql_fetch_row(res)) != NULL) {
      //sj = sj_add(atol(row[0]));
      client = client_get_by_id(row[1], g_clients);

      do {
         if (client && *client->sbuf == NULL) {

            strncpy(client->sbuf, row[2], sizeof (client->sbuf) - 1);
            snprintf(sql, 1020, "UPDATE %s SET status=5 WHERE id='%s'", opt.db_task_table, row[0]);
            if (opt.debug_log)
               _log("[Notice] data_refresh: sql: %s", sql);

            if (mysql_query(&g_dbc[1], sql)) {
               handle_mysql_error();
               goto end;
            }
         }
      } while (client && (client = client_get_by_id(row[1], client->next)));
   }

end:
   if (res) {
      mysql_free_result(res);
      res = NULL;
   }

   return 1;
}

int communicator_loop() {
   client_t *c;

   int r, len, i;

   unsigned int size;

   unsigned long time_current, ms, sql_t = 0, chk_t = 0;

   struct timespec tms_current, tms_sql;

   char buf[1024] = {0};

   while (1) {
      clock_gettime(CLOCK_MONOTONIC_RAW, &tms_current);

      if (timespec_diff_ms(&tms_current, &tms_sql) >= 200) {
         data_refresh();
         if (g_dbc_error)
            return 0;
         tms_sql = tms_current;
      }

      time_current = time(NULL);
      srand(time_current);

      c = g_clients;

      while (c) {

         if (g_dbc_error)
            return 0;

         //_log("%u %u %u", c->ip, c->sock, c->time_connected);
         if (c->active) {
            if (sizeof (c->rbuf) - 1 > c->rn)
               size = sizeof (c->rbuf) - c->rn - 1;
            else
               size = sizeof (c->rbuf) - 1;
            if ((r = recv(c->sock, c->rbuf + c->rn, size, 0)) > 0) {
               c->rn += r;
               c->rbuf[c->rn] = NULL;
               _log("BUF: %s", c->rbuf);

            } else if (errno != EAGAIN) {
               if (opt.debug_log)
                  _log("[Notice] communicator_loop: (errno!=EAGAIN)");

               client_operation(c, 1);
               close(c->sock);
               c->active = 0;
               continue;
            } else if (!c->initialized && time_current - c->time_connected > UNINITIALIZED_CLIENT_TIMEOUT_S) {
               if (opt.debug_log)
                  _log("[Notice] communicator_loop: Uninitialized client timed out. Disconnecting it.");

               client_operation(c, 1);
               close(c->sock);
               c->active = 0;
               continue;
            }


            if (c->initialized && c->rn >= 2 && c->rbuf[c->rn - 1] == '\n') {
               c->rbuf[c->rn - 1] = NULL;
               if (c->rbuf[c->rn - 2] == '\r')
                  c->rbuf[c->rn - 2] = NULL;

               if (c->rn > 2)
                  client_operation(c, 2);

               c->rn = 0;
            }

            if (!c->initialized && c->rn > 4 && c->rbuf[c->rn - 1] == '\n') {
               c->rbuf[c->rn - 1] = NULL;
               if (c->rbuf[c->rn - 2] == '\r')
                  c->rbuf[c->rn - 2] = NULL;

               client_initialize(c);
               c->rn = 0;
            }

            if (c->initialized && time_current > c->time_checked) {
               socksend(c->sock, (unsigned char*) "P", 1, 1);
               c->time_checked = time_current + 30;
            }

            if (*c->sbuf != 0) {
               if (opt.debug_log)
                  _log("[Notice] communicator_loop: Sending sbuf. Data: %s", c->sbuf);

               i = strlen(c->sbuf);
               c->sbuf[i] = '\r';
               c->sbuf[i + 1] = '\n';
               socksend(c->sock, (unsigned char*) c->sbuf, i + 2, 1);
               *c->sbuf = 0;
            }

            if (c->rn > 120)
               c->rn = 0;
         }

         c = c->next;
      }

      usleep(1000 * 50);
   }

   return 1;
}

int db_connect() {
   char error;
   int i;
   error = 0;
   g_dbc_error = 0;

   _log("Connecting to mysql server..");
   for (i = 0; i < NUM_MYSQL_CONNECTIONS; i++) {

      mysql_init(&g_dbc[i]);

      // Connect to database
      if (!mysql_real_connect(&g_dbc[i], opt.db_host, opt.db_user, opt.db_password, opt.db_name, opt.db_port, NULL, 0)) {
         error = 1;
         //sleep(10000);
      }

   }

   if (error) {
      mysql_error_report_all();
      return 0;
   }

   return 1;
}

int db_disconnect() {
   int i;
   _log("Disconnecting from mysql server..");
   for (i = 0; i < NUM_MYSQL_CONNECTIONS; i++) {
      mysql_close(&g_dbc[i]);
   }
   return 1;
}