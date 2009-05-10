/* Copyright (c) 2007 River Tarnell <river@attenuate.org>. */
/*
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely. This software is provided 'as-is', without any express or implied
 * warranty.
 */

/* $Id$ */

#if defined(__sun) && defined(__SVR4)
# define HAVE_UTMPX
#elif defined(__linux__)
# define HAVE_UTMPX
#endif

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<string.h>
#include	<fcntl.h>
#include	<stdio.h>
#include	<unistd.h>
#ifdef HAVE_UTMPX
# include	<utmpx.h>
#else
# include	<utmp.h>
# define ut_user ut_name
#endif

int
get_user_tty(user)
	char const *user;
{
char		 ttydev[32];
struct stat	 st;
time_t		 idle = 0;
int		 fd = -1;
#ifndef HAVE_UTMPX
int		 utfd = -1;
struct utmp	 uti;
struct utmp	*ut = &uti;
	if ((utfd = open(_PATH_UTMP, O_RDONLY)) == -1)
		return -1;

	while (read(utfd, (char *) ut, sizeof(uti)) == sizeof(uti)) {
#else
struct utmpx	*ut;
	setutxent();
	while ((ut = getutxent()) != NULL) {
#endif

	int	tmp;
#ifdef HAVE_UTMPX
		if (ut->ut_type != USER_PROCESS)
			continue;
#else
		if (!ut->ut_host[0])
			continue;
#endif

		if (strcmp(ut->ut_user, user))
			continue;

		(void) snprintf(ttydev, sizeof(ttydev), "/dev/%s", ut->ut_line);
		if ((tmp = open(ttydev, O_WRONLY)) == -1)
			continue;

		if (fstat(tmp, &st) == -1) {
			(void) close(tmp);
			continue;
		}

		if (st.st_atime > idle) {
			fd = tmp;
			idle = st.st_atime;
		} else {
			(void) close(tmp);
		}
	}

	return fd;
}

#ifdef TEST
int
main() {
	sleep(5);
	int i = get_user_tty("river");
	if (i == -1) {
		perror("get_user_tty");
		return 0;
	}
#define MESSAGE "\n\ntesting!\n\n"
	write(i, MESSAGE, sizeof(MESSAGE) - 1);
	close(i);
	return 0;
}
#endif	/* TEST */
