/*
 * Print account expiry date.
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

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#ifdef USE_GETSPNAM
# include <shadow.h>
#endif

int
main(argc, argv)
	int argc;
	char **argv;
{
struct spwd	*spwent;
struct passwd	*pwent;
char const	*uname = NULL;
uid_t		 uid;
time_t		 when;
char		 res[256];
struct tm	 *whentm;
	if (argc > 1) {
		if (getuid() > 0) {
			(void) fprintf(stderr, "only the super-user may view"
				" the account expiry data of another user\n");
			return 1;
		}

		uname = argv[1];
	} else {
		uid = getuid();
		if ((pwent = getpwuid(uid)) == NULL) {
			(void) fprintf(stderr, "getpwuid: %s\n", strerror(errno));
			return 1;
		}

		uname = pwent->pw_name;
	}
	if ((spwent = getspnam(uname)) == NULL) {
		(void) fprintf(stderr, "getspnam: %s\n", strerror(errno));
		return 1;
	}
	(void) setuid(getuid());
	if (spwent->sp_expire == 0) {
		return 0;
	}
	when = spwent->sp_expire * 24 * 60 * 60;
	if (when < 0) {
		return 0;
	}
	if ((whentm = localtime(&when)) == NULL) {
		(void) fprintf(stderr, "localtime: %s\n", strerror(errno));
		return 1;
	}
	(void) strftime(res, sizeof(res) - 1, "%A, %d %B %Y", whentm);
	res[sizeof(res) - 1] = '\0';
	if (argc > 1)
		(void) printf("The account \"%s\" will expire on %s.\n", uname, res);
	else
		(void) printf("Your account will expire on %s.\n", res);
	return 0;
}
