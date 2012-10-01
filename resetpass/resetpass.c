/*
 *  Copyright (c) 2008, River Tarnell
 *  Copyright (c) 2010, Wikimedia Deutschland (River Tarnell)
 *  Copyright (c) 2012, Tim Landscheidt
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

/*
 * resetpass: allow user to reset forgotten LDAP password
 */

#define _GNU_SOURCE
#define LDAP_DEPRECATED 1

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/wait.h>
#include	<errno.h>
#include	<string.h>
#include	<stdio.h>
#include	<unistd.h>
#include	<pwd.h>
#include	<utmpx.h>
#include	<stdarg.h>
#include	<fcntl.h>
#include	<inttypes.h>
#include	<time.h>
#include	<strings.h>
#include	<ldap.h>
#include	<priv.h>

#include	"tsutils.h"

static const char *mktoken(void);
static int check_utmp(const char *username);
static int set_password(LDAP *conn, const char *username);
static int validate_token(const char *username, const char *email);
static int generate_token(const char *username, const char *email);
/* Like setuid and setgid, but with error checking */
static void xroot(void);
static void xuser(void);

#define TOKENDIR "/var/resetpass"
#define TOKENSZ 32 /* Maximum length, not actual length */
#define TOKENCHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

static void
xroot(void)
{
	if (setegid(0) == -1) {
		perror("resetpass: internal error (xroot)");
		_exit(1);
	}

	if (seteuid(0) == -1) {
		perror("resetpass: internal error (xroot)");
		_exit(1);
	}
}

static void
xuser(void)
{
	if (setegid(getgid()) == -1) {
		perror("resetpass: internal error (xroot)");
		_exit(1);
	}

	if (seteuid(getuid()) == -1) {
		perror("resetpass: internal error (xroot)");
		_exit(1);
	}
}

static const char *
mktoken()
{
int		f = -1, i, t;
unsigned char	data[12];
static char	token[TOKENSZ + 1];

	if ((f = open("/dev/urandom", O_RDONLY)) == -1) {
		perror("/dev/urandom");
		return NULL;
	}

	if (read(f, data, sizeof(data)) < (ssize_t) sizeof(data)) 
		goto err;

	/*
	 * This is a naive base64 implementation.  It cannot handle input
	 * which is not a multiple of 3 bytes, but since data is always 12
	 * bytes, this isn't a problem.
	 */
        for (i = 0, t = 0; i < (ssize_t) sizeof(data); i += 3, t += 4) {
        uint32_t pack = (data[i] << 16) | (data[i + 1] << 8) | (data[i + 2]);

                token[t] = TOKENCHARS[(pack >> 18) & 0x3F];
                token[t + 1] = TOKENCHARS[(pack >> 12) & 0x3F];
                token[t + 2] = TOKENCHARS[(pack >> 6) & 0x3F];
                token[t + 3] = TOKENCHARS[pack & 0x3F];
        }

	(void) close(f);
	return token;
err:
	if (f != -1)
		(void) close(f);
	return NULL;
}

static int
check_utmp(username)
	const char *username;
{
struct utmpx	*utent;
char		*tty;
	if ((tty = ttyname(0)) == NULL)
		return 0;

	if (!strncmp(tty, "/dev/", 5))
		tty += 5;

	while ((utent = getutxent()) != NULL) {
		if (utent->ut_type != USER_PROCESS)
			continue;
		if (!utent->ut_line || strcmp(utent->ut_line, tty))
			continue;
		if (strcmp(utent->ut_user, username))
			continue;
		return 1;
	}
	return 0;
}

int
main(argc, argv)
	int argc;
	char **argv;
{
LDAP		*conn;
struct passwd	*pwd;
char		*email, *curpass;

	(void) argv;

	/* Create the token directory while we're root */
	umask(07);

	if (mkdir(TOKENDIR, 0770) == -1 && errno != EEXIST) {
		perror("resetpass: internal error");
		return 1;
	}

	if ((conn = ldap_connect_priv()) == NULL)
		return 1;

	/* Drop privs */
	xuser();

	if (argc != 1) {
		(void) fprintf(stderr, "usage: resetpass\n");
		return 1;
	}

	if (!isatty(0) || !isatty(1) || !isatty(2)) {
		(void) fprintf(stderr, "resetpass: must be run from a terminal\n");
		return 1;
	}

	if ((pwd = getpwuid(getuid())) == NULL) {
		(void) fprintf(stderr, "resetpass: you don't exist\n");
		return 1;
	}

	if (!check_utmp(pwd->pw_name)) {
		(void) fprintf(stderr, "resetpass: you don't seem to be logged in\n");
		return 1;
	}

	if (ldap_user_get_attr(conn, pwd->pw_name, "userPassword", &curpass) < 0)
		return 1;

	if (!curpass || !strcmp(curpass, "{crypt}!")) {
		(void) fprintf(stderr, "resetpass: password not set\n");
		(void) fprintf(stderr, "resetpass: use setpass(1) to set your password\n");
		return 1;
	}

	bzero(curpass, strlen(curpass));
	free(curpass);

	if (ldap_user_get_attr(conn, pwd->pw_name, "mail", &email) < 0)
		return 1;

	if (!email) {
		(void) fprintf(stderr,
			"resetpass: you don't have an email address set.\n");
		(void) fprintf(stderr,
			"resetpass: use setmail(1) before running this program.\n");
		return 1;
	}

	if (!validate_token(pwd->pw_name, email))
		return 0;
	else
		if (set_password(conn, pwd->pw_name) != 0) {
			(void) fprintf(stderr, "resetpass: re-run to try again\n");
			return 1;
		} else {
		int	d;
			/* Remove the token */
			xroot();
			if ((d = open(TOKENDIR, O_RDONLY)) == -1) {
				xuser();
				perror("resetpass: internal error (remove token)");
				return 1;
			}

			unlinkat(d, pwd->pw_name, 0);
			xuser();
		}

	return 0;
}

static int
set_password(conn, username)
	LDAP		*conn;
	const char	*username;
{
char		 newpass[128], verify[128], *s;

	if ((s = ts_getpass("Enter new password: ")) == NULL)
		return 1;
	strlcpy(newpass, s, sizeof(newpass));

	if ((s = ts_getpass("Re-enter password: ")) == NULL)
		return 1;
	strlcpy(verify, s, sizeof(verify));

	if (strcmp(newpass, verify)) {
		(void) fprintf(stderr, "resetpass: passwords don't match\n");
		return 1;
	}

	if (ldap_user_replace_attr(conn, username, "userPassword", newpass) < 0)
		return 1;

	(void) fprintf(stderr, "resetpass: new password successfully set\n");
	return 0;
}

static int
validate_token(username, email)
	const char	*username, *email;
{
struct stat	 sb;
struct tm	*tm;
char		 tfmt[128];
int		 d = -1, f = -1;
char		 token[TOKENSZ + 1] = { 0 };

	xroot();
	if ((d = open(TOKENDIR, O_RDONLY)) == -1) {
		xuser();
		perror("resetpass: internal error (open tokendir)");
		goto err;
	}

	if ((f = openat(d, username, O_RDONLY)) == -1) {
		xuser();
		if (errno != ENOENT) {
			perror("resetpass: internal error (read token)");
			goto err;
		}

		/*
		 * Token doesn't exist; generate one and mail it to
		 * the user.
		 */
		if (generate_token(username, email) < 0) {
			(void) fprintf(stderr, "resetpass: internal error sending token\n");
			goto err;
		}

		return 0;
	} else {
	char	*try;
		/* Found the token; verify the user knows it. */

		if (fstat(f, &sb) == -1) {
			xuser();
			perror("resetpass: internal error (fstat)");
			goto err;
		}

		if (read(f, token, TOKENSZ) < 0) {
			xuser();
			perror("resetpass: internal error (read)");
			goto err;
		}
		xuser();

		tm = gmtime(&sb.st_mtime);
		strftime(tfmt, sizeof (tfmt), "%d-%b-%Y %H:%M:%S", tm);
		(void) printf(
"\nA password reset token was emailed to you on %s.\n"
"If you did not receive this token, press enter to generate a new token.\n\n",
			tfmt
		);

		if ((try = ts_getpass("Password reset token: ")) == NULL)
			goto err;

		printf("\n");

		if (!*try) {
		int	i;
			xroot();
			i = unlinkat(d, username, 0);
			xuser();

			if (i == -1) {
				perror("resetpass: internal error removing token");
				goto err;
			}

			if (generate_token(username, email) < 0)
				perror("resetpass: internal error sending token");
			goto err;
		}

		if (strcmp(try, token)) {
			(void) printf("resetpass: incorrect token.\n");
			goto err;
		}

		close(f);
		close(d);
		return 1;
	}

err:
	if (d != -1)
		close(d);
	if (f != -1)
		close(f);
	return 0;
}

static int
generate_token(username, email)
	const char	*username;
	const char	*email;
{
int		 d = -1, f = -1;
const char	*ntok;
int		 fds[2];
uid_t		 olduid;
char		*msg;
int		 status, pid;

	if ((ntok = mktoken()) == NULL) {
		perror("resetpass: internal error (create token)");
		goto err;
	}

	xroot();
	if ((d = open(TOKENDIR, O_RDONLY)) == -1) {
		xuser();
		perror("resetpass: internal error (open token directory)");
		goto err;
	}

	if ((f = openat(d, username, O_WRONLY | O_CREAT | O_EXCL, 0770)) == -1) {
		xuser();
		perror("resetpass: internal error (open token)");
		goto err;
	}
	xuser();

	if (write(f, ntok, strlen(ntok)) < (ssize_t) strlen(ntok)) {
		perror("resetpass: internal error (write token)");
		unlinkat(d, username, 0);
	}

	if (pipe(fds) == -1) {
		perror("resetpass: internal error (pipe)");
		goto err;
	}

	olduid = getuid();
	/*
	 * We need root access back to send the password mail, so that
	 * sendmail runs as root and can't be intercepted by the user.
	 */
	if (setuid(0) == -1) {
		perror("resetpass: internal error (setuid)");
		_exit(1);
	}

	pid = fork();

	switch (pid) {
	case 0: { /* child */
	char const * const env[] = {
		"PATH=/usr/bin",
		NULL
	};
	char const * const args[] = {
		"/usr/lib/sendmail",
		"-oi", "-bm", "--", email,
		NULL
	};

		setgid(0);

		chdir("/");
		dup2(fds[0], 0);
		close(fds[0]);
		close(fds[1]);

		execve("/usr/lib/sendmail", (char *const *) args, (char *const *) env);
		_exit(1);
	}

	default:  /* parent */
		  break;

	case -1: /* error */
		perror("resetpass: internal error (fork)");
		goto err;
	}

	/* Drop root again */
	if (seteuid(olduid) == -1) {
		perror("resetpass: internal error (seteuid)");
		_exit(1);
	}

	asprintf(&msg,
"From: root <ts-admins@toolserver.org>\n"
"To: %s <%s>\n"
"Subject: Toolserver password reset token\n"
"\n"
"Hello,\n"
"\n"
"Someone, hopefully you, has requested that your Toolserver password\n"
"be reset.  Your password reset token is:\n"
"\n"
"        %s\n"
"\n"
"To reset your password, run \"resetpass\" again, and enter this token.\n"
"\n"
"Regards,\n"
"  the automatically-reset-your-password program.\n",
	username, email, ntok);

	close(fds[0]);
	write(fds[1], msg, strlen(msg));
	close(fds[1]);

	wait(&status);
	if ((WIFEXITED(status) && (WEXITSTATUS(status) != 0)) ||
	    WIFSIGNALED(status)) {
		(void) fprintf(stderr, "resetpass: sending token failed\n");
		goto err;
	}

	(void) printf(
"A new password reset token has been generated and mailed to you.  When you\n"
"receive the token, re-run resetpass.\n");

	close(f);
	close(d);
	return 0;

err:
	if (d != -1)
		close(d);
	if (f != -1)
		close(f);
	return -1;
}
