/* 
 * Copyright (C) 2013 Martynas Bagdonas
 * 
 * http://martynas.bagdonas.net/
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */

#ifndef LOG_H
#define	LOG_H


#ifdef ENABLE_LOGGING
void _log(const char *log_str, ...);
#else
#define _log  
#endif


#endif	/* LOG_H */

