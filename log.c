/* 
 * Copyright (C) 2013 Martynas Bagdonas
 * 
 * http://martynas.bagdonas.net/
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */

#include <time.h>
#include <stdarg.h>
#include "string.h"
#include <stdio.h>
#include <mysql/mysql.h>

#include "defines.h"
#include "log.h"
#include "main.h"


#ifdef ENABLE_LOGGING

void _log(const char *log_str, ...) {
   int itmp = 0;
   static int new_session = 1;
   char buf[2048] = {0};
   va_list va_alist;
   long ti;
   struct tm *tm;
   FILE *p_file;



   time(&ti);
   tm = localtime(&ti);

   if (new_session) {
      new_session = 0;
      snprintf(buf, sizeof (buf) - 1, "\r\n\r\n$#$#$#$#$ <O> <o> SESSION STARTED [%.2d:%.2d:%.2d] <o> <O> $#$#$#$#$\r\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
   }


   va_start(va_alist, log_str);
   vsprintf(&buf[strlen(buf)], log_str, va_alist);
   va_end(va_alist);

   strcat(buf, "\r\n");

   if (strlen(opt.log_file) > 0) {
      p_file = fopen(opt.log_file, "w");
      if (p_file != NULL) {
         fputs(buf, p_file);
         fclose(p_file);
      }
   }

   if (opt.console)
      printf("%s", buf);
}
#endif
