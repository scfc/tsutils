/* Copyright (c) 2007 River Tarnell <river@attenuate.org>. */
/*
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely. This software is provided 'as-is', without any express or implied
 * warranty.
 */

/* $Id$ */

/*
 * listlogins: report other login sessions from this user.
 */

#if defined(__sun) && defined(__SVR4)
# define SOLARIS
#endif

#if defined(SOLARIS) || defined(__linux__)
# define HAVE_UTMPX
#endif

#include	<sys/types.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<pwd.h>
#include	<fcntl.h>
#ifdef HAVE_UTMPX
# include	<utmpx.h>
#else
# include	<utmp.h>
# define ut_user ut_name
#endif

typedef struct ttyent {
	struct ttyent	*te_next;
	char		 te_name[16];
} ttyent_t;

typedef struct utent {
	struct utent	*ute_next;
	char		 ute_host[256];
	ttyent_t	 ute_lines;
} utent_t;

int
main()
{
struct passwd	*pwd;
utent_t		 ents, *utp;
ttyent_t	*ttye;
char		*tty = ttyname(0);
#ifdef HAVE_UTMPX
struct utmpx	*ut;
#else
struct utmp	 uti;
struct utmp	*ut = &uti;
int		 utfd;
#endif

	if (strlen(tty) < 5)
		tty = 0;

	if ((pwd = getpwuid(getuid())) == NULL)
		return 0;

	(void) memset(&ents, 0, sizeof(ents));

#ifdef HAVE_UTMPX
	setutxent();
	while ((ut = getutxent()) != NULL) {
#else
	if ((utfd = open(_PATH_UTMP, O_RDONLY)) == -1)
		return 0;
	while (read(utfd, (char *) ut, sizeof(uti)) == sizeof(uti)) {
#endif

#ifdef HAVE_UTMPX
		if (ut->ut_type != USER_PROCESS)
			continue;
#else
		if (!ut->ut_host[0])
			continue;
#endif

		if (strcmp(ut->ut_user, pwd->pw_name))
			continue;

		if (tty && !strcmp(ut->ut_line, tty + 5))
			continue;

		utp = NULL;
		/* If we already saw this host, add the line to the list */
		for (utp = ents.ute_next; utp; utp = utp->ute_next) {
			if (!strcmp(utp->ute_host, ut->ut_host))
				break;
		}

		if (utp == NULL) {
			utp = calloc(1, sizeof(*utp));
			(void) strncpy(utp->ute_host, ut->ut_host, sizeof(utp->ute_host) - 1);
			if (ut->ut_host[sizeof(ut->ut_host) - 1] != 0)
				utp->ute_host[sizeof(ut->ut_host) - 1] = 0;
			utp->ute_next = ents.ute_next;
			ents.ute_next = utp;
		}

		ttye = calloc(1, sizeof(*ttye));
		(void) strncpy(ttye->te_name, ut->ut_line, sizeof(ttye->te_name) - 1);
		ttye->te_next = utp->ute_lines.te_next;
		utp->ute_lines.te_next = ttye;
	}
#ifdef HAVE_UTMPX
	endutxent();
#else	
	close(utfd);
#endif

	if (ents.ute_next)
		(void) printf("You are already logged in from the following host(s):\n");

	for (utp = ents.ute_next; utp; utp = utp->ute_next) {
		(void) printf("  %s (", utp->ute_host);
		for (ttye = utp->ute_lines.te_next; ttye; ttye = ttye->te_next) {
			(void) printf("%s", ttye->te_name);
			if (ttye->te_next)
				(void) printf(", ");
		}
		(void) printf(")\n");
	}
	return 0;
}
