/*
 *  Copyright (c) 2009, River Tarnell
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
 * setmail: allow users to change their LDAP 'mail' attribute
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
char		*input, *newmail, *curmail;

	if ((pwd = getpwuid(getuid())) == NULL) {
		(void) fprintf(stderr, "setmail: you don't seem to exist\n");
		return 1;
	}

	if ((conn = ldap_connect()) == NULL)
		return 1;

	if (ldap_user_get_attr(conn, pwd->pw_name, "mail", &curmail) < 0)
		return 1;

	if (!curmail)
		(void) printf("\nNo email address is currently set.\n");
	else
		(void) printf("\nCurrent email address: %s\n", curmail);

	if ((input = readline("Do you wish to change this address? (yes/no) ")) == NULL)
		return 1;

	if (strcmp(input, "yes"))
		return 0;

	(void) printf("\n");
	if ((newmail = readline("Enter new e-mail address: ")) == NULL)
		return 1;

	(void) printf("\nYour new email address is: %s\n", newmail);
	if ((input = readline("Continue? (yes/no) ")) == NULL)
		return 1;
	if (strcmp(input, "yes"))
		return 0;

	if (ldap_user_replace_attr(conn, pwd->pw_name, "mail", newmail) < 0)
		return 1;

	(void) printf("setmail: new address successfully set\n");
	return 0;
}
