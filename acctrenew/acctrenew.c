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
 * acctrenew: allow users to renew their accounts
 */

#include	<errno.h>
#include	<string.h>
#include	<stdio.h>
#include	<unistd.h>
#include	<pwd.h>
#include	<utmpx.h>
#include	<stdarg.h>
#include	<priv.h>
#include	<ldap.h>
#include	<shadow.h>

#include	<readline/readline.h>

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
struct spwd	*spwd;
time_t		 n;
struct tm	*tm;
char		 tfmt[128];
char		*answer;
char		*email, *license;

	(void) argv;

	if (argc != 1) {
		(void) fprintf(stderr, "usage: acctrenew\n");
		return 1;
	}

	/*
	 * Drop all privileges, but leave DAC read in the permitted set.  This
	 * is needed to read the LDAP password.
	 */
	if (priv_set(PRIV_SET, PRIV_PERMITTED, PRIV_FILE_DAC_READ, PRIV_PROC_FORK, PRIV_PROC_EXEC, NULL) == -1 ||
	    priv_set(PRIV_SET, PRIV_INHERITABLE, PRIV_PROC_FORK, PRIV_PROC_EXEC, NULL) == -1 ||
	    priv_set(PRIV_SET, PRIV_EFFECTIVE, NULL) == -1) {
		perror("acctrenew: priv_set");
		return 1;
	}

	if ((pwd = getpwuid(getuid())) == NULL) {
		perror("acctrenew: getpwuid");
		return 1;
	}

	if ((spwd = getspnam(pwd->pw_name)) == NULL) {
		perror("acctrenew: getspnam");
		return 1;
	}

	if (setuid(getuid()) == -1) {
		perror("acctrenew: setuid");
		return 1;
	}

	if ((conn = ldap_connect_priv()) == NULL)
		return 1;

	/* We no longer need any privileges */
	priv_set(PRIV_SET, PRIV_PERMITTED, PRIV_PROC_FORK, PRIV_PROC_EXEC, NULL);
	priv_set(PRIV_SET, PRIV_EFFECTIVE, PRIV_PROC_FORK, PRIV_PROC_EXEC, NULL);

	if (!isatty(0) || !isatty(1) || !isatty(2)) {
		(void) fprintf(stderr, "acctrenew: must be run from a terminal\n");
		return 1;
	}

	if (!check_utmp(pwd->pw_name)) {
		(void) fprintf(stderr, "acctrenew: you don't seem to be logged in\n");
		return 1;
	}

	n = spwd->sp_expire * 24 * 60 * 60;
	tm = localtime(&n);
	strftime(tfmt, sizeof (tfmt), "%d-%b-%Y", tm);

	/*
	 * Only allow renewal if the account expiry date is
	 * sooner than 29 days.
	 */
	if (((n / 60 / 60 / 24) - (time(NULL) / 60 / 60 / 24)) >= 29) {
		(void) fprintf(stderr, 
			"acctrenew: you may only renew your "
			"account 28 days before expiry\n");
		return 1;
	}

	printf(
"Toolserver account renewal tool\n"
"===============================\n"
"\n"
"This utility will extend the expiry date of your account by\n"
"six months.  It should be used when your account is nearing\n"
"expiry.\n"
"\n"
"Current account expiry date: %s\n"
"\n", tfmt);

	if ((answer = readline("Do you wish to continue (yes/no)? ")) == NULL)
		return 0;
	if (*answer != 'y')
		return 0;

	printf("\n");
	/* Authenticate the user by attempting a bind */
	if (ldap_connect() == NULL)
		return 1;

	printf(
"\n"
"All Toolserver users are required to abide by the rules, which\n"
"are listed at <https://wiki.toolserver.org/view/Rules>.  The\n"
"rules may change from time to time, so you should now take a\n"
"moment to review them.\n"
"\n");

	if ((answer = readline("Do you agree to the rules as currently listed (yes/no)? ")) == NULL)
		return 0;

	if (*answer != 'y')
		return 0;

	printf(
"\n"
"All Toolserver accounts should have an up-to-date email address\n"
"so that the Toolserver administrators can contact them.\n\n");

	if (ldap_user_get_attr(conn, pwd->pw_name, "mail", &email) < 0)
		return 1;

	if (!email) {
		(void) printf(
"No email address is currently set for your account in LDAP.  If\n"
"you like, I can invoke \"setmail\" for you now, so you can set\n"
"an address.\n\n");
	} else {
		(void) printf("Current email address: %s\n\n", email);
		(void) printf(
"If this address is not correct, I can invoke \"setmail\" for you\n"
"now, so you can change it.\n\n");
	}

	for (;;) {
	char	*answer;
	int	 i;
		if ((answer = readline("Invoke setmail (yes/no)? ")) == NULL)
			return 0;

		if (*answer != 'y')
			break;

		printf("\n=== setmail starts ===\n");
		i = system("/opt/local/bin/setmail");
		printf("=== setmail ends ===\n\n");

		if (i)
			printf("It looks like setmail failed for some reason.\n");
		else
			break;
	}

	printf(
"\nToolserver users are able to set a default license to be used for\n"
"their tools, when there is no explicit license on the tool.  You\n"
"can read more about this at <https://wiki.toolserver.org/view/Default_license>.\n\n"
);

	if (ldap_user_get_attr(conn, pwd->pw_name, "tsDefaultLicense", &license) < 0)
		return 1;

	if (!license) {
		(void) printf(
"Your account currently has no default license.  If you like, I\n"
"can invoke \"setlicense\" for you now, so you can set a license.\n\n");
	} else {
		(void) printf("Current default license: %s\n\n", license);
		(void) printf(
"If this license is not correct, I can invoke \"setlicense\" for you\n"
"now, so you can change it.\n\n");
	}

	for (;;) {
	char	*answer;
	int	 i;
		if ((answer = readline("Invoke setlicense (yes/no)? ")) == NULL)
			return 0;

		if (*answer != 'y')
			break;

		printf("\n=== setlicense starts ===\n");
		i = system("/opt/local/bin/setlicense");
		printf("=== setlicense ends ===\n\n");

		if (i)
			printf("It looks like setlicense failed for some reason.\n");
		else
			break;
	}

	printf("\nOkay, that's everything.  Please wait while I renew your account...\n");

	n = time(NULL);
	n /= (60 * 60 * 24);
	/* 6 months is roughly 24 weeks */
	n += 24 * 7;
	sprintf(tfmt, "%d", n);

	if (ldap_user_replace_attr(conn, pwd->pw_name, "shadowExpire", tfmt) < 0)
		return 1;
	
	printf("Okay, all done.\n");
	return 0;
}
