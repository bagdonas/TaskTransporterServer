/* 
 * Copyright (C) 2013 Martynas Bagdonas
 * 
 * http://martynas.bagdonas.net/
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */

/*
 * Run those two commands to get required op codes for using mysql library
 * mysql_config --cflags
 * mysql_config --libs
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <mysql/mysql.h>


#include "defines.h"
#include "log.h"
#include "client.h"
#include "server.h"
#include "communicator.h"
#include "main.h"

options_t opt = {0, 0, 1, "", 4300, "localhost", 0, "task_transporter", "root", "", "task", "user", "id", "session_hash", "is_connected"};

int daemonize() {
   /* Our process ID and Session ID */
   pid_t pid, sid;

   /* Fork off the parent process */
   pid = fork();
   if (pid < 0) {
      exit(EXIT_FAILURE);
   }
   /* If we got a good PID, then
      we can exit the parent process. */
   if (pid > 0) {
      exit(EXIT_SUCCESS);
   }

   /* Change the file mode mask */
   umask(0);

   /* Open any logs here */

   /* Create a new SID for the child process */
   sid = setsid();
   if (sid < 0) {
      /* Log the failure */
      exit(EXIT_FAILURE);
   }

   /* Change the current working directory */
   if ((chdir("/")) < 0) {
      /* Log the failure */
      exit(EXIT_FAILURE);
   }

   /* Close out the standard file descriptors */
   if (!opt.console) {
      close(STDIN_FILENO);
      close(STDOUT_FILENO);
      close(STDERR_FILENO);
   }

   return 0;
}

void usage() {
   printf(
           "--daemon - run as daemon\n"
           "--debug - enable debug logs\n"
           "--console - output logs to console\n"
           "-l, -log_file - select logs output file\n"
           "-s, -server_port - server listening port\n"
           "-h, -db_host - database host name or ip address. Default: \"localhost\"\n"
           "-m, -db_port - database port. Default: 3306\n"
           "-n, -db_name - database name. Default: \"task_transporter\"\n"
           "-u, -db_user - database user. Default: \"root\"\n"
           "-p, -db_password - database password, leave empty to connect without password. Default: \"\"\n"
           "Example:\n"
           "tasktransporter -s 4300 -h localhost -n task_transporter -u root -p mypassword --daemon\n"
           );
}

int init_options(int argc, char **argv) {
   int c;
   char *p;

   while (1) {
      static struct option long_options[] = {
         /* These options set a flag. */
         {"daemon", no_argument, &opt.daemon, 1},
         {"debug", no_argument, &opt.debug_log, 1},
         {"console", no_argument, &opt.console, 1},
         /* These options don't set a flag.
            We distinguish them by their indices. */
         {"log_file", required_argument, 0, 'l'},
         {"server_port", required_argument, 0, 's'},
         {"db_host", required_argument, 0, 'h'},
         {"db_port", required_argument, 0, 'm'},
         {"db_name", required_argument, 0, 'n'},
         {"db_user", required_argument, 0, 'u'},
         {"db_password", required_argument, 0, 'p'},
         {"db_task_table", required_argument, 0, 'T'},
         {"db_user_table", required_argument, 0, 'U'},
         {"db_userid_field", required_argument, 0, 'I'},
         {"db_session_field", required_argument, 0, 'S'},
         {"db_connected_field", required_argument, 0, 'C'},
         {0, 0, 0, 0}
      };
      /* getopt_long stores the option index here. */
      int option_index = 0;

      c = getopt_long(argc, argv, "l:s:h:m:n:u:p:T:U:I:S:C:",
              long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
         break;

      switch (c) {
         case 0:
            /* If this option set a flag, do nothing else now. */
            if (long_options[option_index].flag != 0)
               break;
            printf("option %s", long_options[option_index].name);
            if (optarg)
               printf(" with arg %s", optarg);
            printf("\n");
            break;

         case 'l':
            strncpy(opt.log_file, optarg, sizeof (opt.log_file) - 1);
            break;

         case 's':
            opt.server_port = atoi(optarg);
            break;

         case 'h':
            strncpy(opt.db_host, optarg, sizeof (opt.db_host) - 1);
            break;

         case 'm':
            opt.db_port = atoi(optarg);
            break;

         case 'n':
            strncpy(opt.db_name, optarg, sizeof (opt.db_name) - 1);
            break;

         case 'u':
            strncpy(opt.db_user, optarg, sizeof (opt.db_user) - 1);
            break;

         case 'p':
            strncpy(opt.db_password, optarg, sizeof (opt.db_password) - 1);
            break;

            /*case 'b':
               p = strtok(optarg, ".");
               if (p) {
                  strncpy(opt.db_session_table, p, sizeof (opt.db_session_table) - 1);
                  p = strtok(NULL, ".");
                  if (p) {
                     strncpy(opt.db_session_field, p, sizeof (opt.db_session_field) - 1);
                  }
               }
               break;*/
         case 'T':
            strncpy(opt.db_task_table, optarg, sizeof (opt.db_task_table) - 1);
            break;

         case 'U':
            strncpy(opt.db_user_table, optarg, sizeof (opt.db_user_table) - 1);
            break;

         case 'I':
            strncpy(opt.db_userid_field, optarg, sizeof (opt.db_userid_field) - 1);
            break;

         case 'S':
            strncpy(opt.db_session_field, optarg, sizeof (opt.db_session_field) - 1);
            break;

         case 'C':
            strncpy(opt.db_connected_field, optarg, sizeof (opt.db_connected_field) - 1);
            break;

         case '?':
            usage();
            abort();
            break;

         default:
            usage();
            abort();
      }
   }
}

int main(int argc, char **argv) {
   unsigned int pid;
   pthread_t th;

   init_options(argc, argv);

   if (opt.daemon)
      daemonize();

   sigignore(SIGPIPE);

   if (mysql_library_init(NULL, 0, 0)) {
      _log("could not initialize MySQL library");
      exit(1);
   }

   pid = getpid();
   _log("PID: %u", pid);


   pthread_create(&th, NULL, server_thread, NULL);
   pthread_detach(th);


   while (1) {
      if (db_connect())
         communicator_loop();
      db_disconnect();
      sleep(2);
   }

   mysql_library_end();

   return 0;
}
