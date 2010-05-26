/*
 * Print all expired accounts.
 * $Id$
 */

#ifdef __FreeBSD__
# define USE_GETPWNAM
# define getspnam getpwnam
# define spwd passwd
# define sp_expire pw_expire
#else
# define USE_GETSPNAM
#endif

#include	<sys/types.h>
#include	<sys/time.h>
#include	<stdio.h>
#include	<pwd.h>
#include	<time.h>
#include	<unistd.h>
#include	<errno.h>
#include	<string.h>
#ifdef USE_GETSPNAM
# include	<shadow.h>
#else
# define getspent getpwent
# define sp_expire pw_expire
# define sp_namp pw_name
# define spwd passwd
#endif

int
main(void)
{
struct spwd	*spwent;
time_t		 when, now;

	(void) time(&now);

	while ((spwent = getspent()) != NULL) {
		if (spwent->sp_expire <= 0)
			continue;

		when = spwent->sp_expire * 24 * 60 * 60;

		if (when > now)
			continue;

		(void) printf("%s\n", spwent->sp_namp);
	}

	return 0;
}
