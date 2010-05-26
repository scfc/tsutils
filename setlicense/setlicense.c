/*
 * Copyright (c) 2010, Wikimedia Deutschland (River Tarnell)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Wikimedia Deutschland nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY WIKIMEDIA DEUTSCHLAND ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL WIKIMEDIA DEUTSCHLAND BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * NOTE: This software is not released as a product. It was written primarily for
 * Wikimedia Deutschland's own use, and is made public as is, in the hope it may
 * be useful. Wikimedia Deutschland may at any time discontinue developing or
 * supporting this software. There is no guarantee any new versions or even fixes
 * for security issues will be released.
 */

/* $Id$ */

/*
 * setlicense: allow users to change their default tool license in LDAP
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
#include	<readline/readline.h>

#include	"tsutils.h"

int
main()
{
LDAP		*conn;
struct passwd	*pwd;
char		*input, *curlic, *newlic;

	if ((pwd = getpwuid(getuid())) == NULL) {
		(void) fprintf(stderr, "setlicense: you don't exist\n");
		return 1;
	}

	(void) printf(
"This program allows you to change your default Toolserver license.  Your\n"
"default license is the license your Toolserver-hosted tools are assumed to\n"
"be under if no other license is specified.\n"
"\n"
"For more information about default licenses, please read\n"
"<https://wiki.toolserver.org/view/Default_license>.\n"
"\n"
	);

	if ((conn = ldap_connect()) == NULL)
		return 1;

	if (ldap_user_get_attr(conn, pwd->pw_name, "tsDefaultLicense", &curlic) < 0)
		return 1;

	if (!curlic)
		(void) printf("\nYou currently have no default license set.\n");
	else
		(void) printf("\nYour current default license is: %s\n", curlic);

	if ((input = readline("Do you wish to change this license? (yes/no) ")) == NULL)
		return 1;

	if (strcmp(input, "yes"))
		return 0;

	(void) printf("\n");
	(void) printf(
"Your default license should be in one of two forms:\n"
"\n"
"  * For a well-known license, use a short name, e.g. \"GPLv2\", \"MIT\" or\n"
"    \"2-clause BSD\"."
"\n"
"  * For a custom or less well-known license, provide a URL to a page that\n"
"    describes your license (this page may be hosted on the Toolserver if\n"
"    you like).\n"
"\n"
	);

	if ((newlic = readline("Enter new default license: ")) == NULL)
		return 1;

	(void) printf("\nYour new default license is: %s\n", newlic);
	if ((input = readline("Continue? (yes/no) ")) == NULL)
		return 1;
	if (strcmp(input, "yes"))
		return 0;

	if (ldap_user_replace_attr(conn, pwd->pw_name, "tsDefaultLicense", newlic) < 0)
		return 1;

	(void) printf("setlicense: new license successfully set\n");
	return 0;
}
