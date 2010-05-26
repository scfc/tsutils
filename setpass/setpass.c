/*
 *  Copyright (c) 2008, River Tarnell.
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

/*
 * setpass: set initial LDAP password.
 */

#define _GNU_SOURCE
#define LDAP_DEPRECATED 1

#include	<errno.h>
#include	<string.h>
#include	<stdio.h>
#include	<unistd.h>
#include	<pwd.h>
#include	<utmpx.h>
#include	<stdarg.h>

#include	<ldap.h>

#include	"tsutils.h"

int
check_utmp(char *username)
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
char		 newpass[128], verify[128], *s;
char		*curpass;

	(void) argv;

	if (argc != 1) {
		(void) fprintf(stderr, "usage: setpass\n");
		return 1;
	}

	/*
	 * Drop all privileges, but leave DAC read in the permitted set.  This
	 * is needed to read the LDAP password.
	 */
	priv_set(PRIV_SET, PRIV_LIMIT, PRIV_FILE_DAC_READ, NULL);
	priv_set(PRIV_SET, PRIV_PERMITTED, PRIV_FILE_DAC_READ, NULL);
	priv_set(PRIV_SET, PRIV_EFFECTIVE, NULL);
	priv_set(PRIV_SET, PRIV_INHERITABLE, NULL);

	if (setuid(getuid()) == -1) {
		perror("setpass: setuid");
		return 1;
	}

	if ((conn = ldap_connect_priv()) == NULL)
		return 1;

	/* We no longer need any privileges */
	priv_set(PRIV_SET, PRIV_LIMIT, NULL);

	if (!isatty(0) || !isatty(1) || !isatty(2)) {
		(void) fprintf(stderr, "setpass: must be run from a terminal\n");
		return 1;
	}

	if ((pwd = getpwuid(getuid())) == NULL) {
		(void) fprintf(stderr, "setpass: you don't exist\n");
		return 1;
	}

	if (!check_utmp(pwd->pw_name)) {
		(void) fprintf(stderr, "setpass: you don't seem to be logged in\n");
		return 1;
	}

	/*
	 * Make sure the user's password is currently {crypt}!, i.e. not set.  
	 * Users who already have a password should use passwd(1) instead of 
	 * this program.
	 */
	if (ldap_user_get_attr(conn, pwd->pw_name, "userPassword", &curpass))
		return 1;

	if (!curpass) {
		(void) fprintf(stderr, "setpass: no result when looking for current userPassword\n");
		return 1;
	}

	if (strcmp(curpass, "{crypt}!")) {
		(void) fprintf(stderr, "setpass: password already set\n");
		(void) fprintf(stderr, "setpass: use passwd(1) to change your password\n");
		return 1;
	}

	if ((s = ts_getpass("Enter new password: ")) == NULL)
		return 1;
	(void) strncpy(newpass, s, sizeof(newpass));
	newpass[sizeof(newpass) - 1] = 0;

	if ((s = ts_getpass("Re-enter password: ")) == NULL)
		return 1;
	(void) strncpy(verify, s, sizeof(verify));
	verify[sizeof(verify) - 1] = 0;

	if (strcmp(newpass, verify)) {
		(void) fprintf(stderr, "setpass: passwords don't match\n");
		return 1;
	}

	if (ldap_user_replace_attr(conn, pwd->pw_name, "userPassword", newpass) == -1)
		return 1;
	else
		return 0;
}
