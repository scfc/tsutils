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

#define SERVER		"ldap.toolserver.org"
#define PORT		LDAP_PORT
#define BASE_DN		"ou=People,o=unix,o=toolserver"

#if defined(__sun) && defined(__SVR4)
# define getpass getpassphrase
#endif

#if defined(__sun) && defined(__SVR4)
int
asprintf(char **strp, char const *fmt, ...)
{
va_list	ap;
int	len;
	va_start(ap, end);
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

char *
get_ldap_secret(void)
{
static	char *secret;
	char *pw;

	if ((pw = getpass("Enter LDAP password: ")) == NULL) {
		(void) fprintf(stderr,
			"setlicense: cannot read LDAP password\n");
		return NULL;
	}

	secret = strdup(pw);
	return secret;
}

int
main()
{
LDAP		*conn;
char		*secret, *userdn;
struct passwd	*pwd;
int		 err;
char		*attrs[2];
LDAPMessage	*result, *ent;
char		**vals;
LDAPMod		 mod;
LDAPMod		*mods[2];
char		*input, *newlicense;

	if ((conn = ldap_init(SERVER, PORT)) == NULL) {
		(void) fprintf(stderr,
			"setlicense: cannot connect to LDAP server: %s:%d: %s\n",
			SERVER, PORT, strerror(errno));
		return 1;
	}

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

	asprintf(&userdn, "uid=%s,%s", pwd->pw_name, BASE_DN);
	
	(void) printf("User DN: %s\n", userdn);

	if ((secret = get_ldap_secret()) == NULL)
		return 1;

	if ((err = ldap_simple_bind_s(conn, userdn, secret)) != 0) {
		(void) fprintf(stderr,
			"setlicense: cannot bind as %s: %s\n",
			userdn, ldap_err2string(err));
		return 1;
	}

	memset(secret, 0, strlen(secret));

	/*
	 * Print the user's existing license.
	 */
	attrs[0] = "tsDefaultLicense";
	attrs[1] = NULL;
	err = ldap_search_s(conn, userdn, LDAP_SCOPE_BASE,
			"(objectclass=posixAccount)",
			attrs, 0, &result);
	if (err) {
		ldap_perror(conn,
			"setlicense: retrieving current license");
		return 1;
	}

	if ((ent = ldap_first_entry(conn, result)) == NULL) {
		(void) fprintf(stderr,
			"setlicense: no result when looking for current license\n");
		return 1;
	}

	if ((vals = ldap_get_values(conn, ent, "tsDefaultLicense")) == NULL
	    || vals[0] == NULL) {
		(void) printf("\nYou currently have no default license set.\n");
	} else
		(void) printf("\nYour current default license is: %s\n", vals[0]);

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

	if ((newlicense = readline("Enter new default license: ")) == NULL)
		return 1;

	(void) printf("\nYour new default license is: %s\n", newlicense);
	if ((input = readline("Continue? (yes/no) ")) == NULL)
		return 1;
	if (strcmp(input, "yes"))
		return 0;

	memset(&mod, 0, sizeof(mod));
	mod.mod_op = LDAP_MOD_REPLACE;
	mod.mod_type = "tsDefaultLicense";
	attrs[0] = newlicense;
	mod.mod_values = attrs;

	mods[0] = &mod;
	mods[1] = NULL;

	if ((err = ldap_modify_s(conn, userdn, mods)) != 0) {
		ldap_perror(conn, "setlicense: setting new license");
		return 1;
	}

	(void) printf("setlicense: new license successfully set\n");
	return 0;
}
