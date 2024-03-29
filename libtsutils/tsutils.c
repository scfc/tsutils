/*
 *  Copyright (c) 2007, River Tarnell
 *  Copyright (c) 2010, Wikimedia Deutschland (River Tarnell)
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *      * Neither the name of Wikimedia Deutschland nor the
 *        names of its contributors may be used to endorse or promote products
 *        derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY WIKIMEDIA DEUTSCHLAND ''AS IS'' AND ANY
 *  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL WIKIMEDIA DEUTSCHLAND BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * NOTE: This software is not released as a product. It was written primarily for
 * Wikimedia Deutschland's own use, and is made public as is, in the hope it may
 * be useful. Wikimedia Deutschland may at any time discontinue developing or
 * supporting this software. There is no guarantee any new versions or even fixes
 * for security issues will be released.
 */

/* $Id$ */

#include	<sys/mman.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<stdlib.h>
#include	<string.h>
#include	<stdarg.h>
#include	<syslog.h>
#include	<stdio.h>
#include	<unistd.h>
#include	"tsutils.h"

char *
realloc_strncat(str, add, len)
	char *str;
	char const *add;
	size_t len;
{
size_t	 newlen;

	newlen = len;
	if (str)
		newlen += strlen(str);
	str = realloc(str, newlen + 1);
	(void) strncat(str, add, len);
	return str;
}

char *
realloc_strcat(str, add)
	char *str;
	char const *add;
{
	return realloc_strncat(str, add, strlen(add));
}

void
strdup_free(s, new)
	char **s;
	char const *new;
{
	if (*s)
		free(*s);

	*s = strdup(new);
}

static int foreground = 1;

void
logmsg(char const *msg, ...)
{
va_list	ap;
	va_start(ap, msg);

	if (foreground) {
		(void) vfprintf(stderr, msg, ap);
		(void) fputs("\n", stderr);
	} else
		vsyslog(LOG_NOTICE, msg, ap);

	va_end(ap);
}

int
daemon_detach(progname)
	char const *progname;
{
	if (daemon(0, 0) < 0)
		return -1;
	openlog(progname, LOG_PID, LOG_DAEMON);
	foreground = 0;
	return 0;
}

char *
file_to_string(path)
	char const *path;
{
int		 i;
void		*addr;
struct stat	 st;
char		*str;

	if ((i = open(path, O_RDONLY)) == -1) {
		return NULL;
	}

	if (fstat(i, &st) == -1) {
		(void) close(i);
		return NULL;
	}

	if ((addr = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, i, 0)) == MAP_FAILED) {
		(void) close(i);
		return NULL;
	}

	str = malloc(st.st_size + 1);
	(void) memcpy(str, addr, st.st_size);
	str[st.st_size] = '\0';

	(void) munmap(addr, st.st_size);
	(void) close(i);

	return str;
}

#if defined(__sun) && defined(__SVR4)
int
asprintf(char **strp, char const *fmt, ...)
{
va_list ap;
int     len;
       va_start(ap, fmt);
       if ((len = vsnprintf(NULL, 0, fmt, ap)) < 0)
               return -1;
       if ((*strp = malloc(len + 1)) == NULL)
               return -1;
       if ((len = vsnprintf(*strp, len + 1, fmt, ap)) < 0) {
               free(*strp);
               return -1;
       }
       return len;
}
#endif
