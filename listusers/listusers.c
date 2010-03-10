/* Copyright (c) 2009 River Tarnell <river@wikimedia.org>. */
/*
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely. This software is provided 'as-is', without any express or implied
 * warranty.
 */

/* $Id$ */

/*
 * listusers: Display list of LDAP users
 */

#define _GNU_SOURCE
#define LDAP_DEPRECATED 1

#include	<errno.h>
#include	<string.h>
#include	<stdio.h>
#include	<unistd.h>
#include	<pwd.h>
#include	<utmpx.h>

#include	<ldap.h>

#include	<readline/readline.h>

#define SERVER		"ldap.toolserver.org"
#define PORT		LDAP_PORT
#define BASE_DN		"ou=People,o=unix,o=toolserver"

char *
get_ldap_secret(void)
{
static	char *secret;
	char *pw;

	if ((pw = getpass("Enter LDAP password: ")) == NULL) {
		(void) fprintf(stderr,
			"listusers: cannot read LDAP password\n");
		return NULL;
	}

	secret = strdup(pw);
	return secret;
}

int
main(argc, argv)
	int argc
#ifdef __GNUC__
	__attribute__((__unused__))
#endif
	;
	char **argv;
{
LDAP		*conn;
char		*secret, *userdn, *searchdn;
struct passwd	*pwd;
int		 err;
char		*attrs[4];
LDAPMessage	*result, *ent;
char		**vals;

	if ((conn = ldap_init(SERVER, PORT)) == NULL) {
		(void) fprintf(stderr,
			"listusers: cannot connect to LDAP server: %s:%d: %s\n",
			SERVER, PORT, strerror(errno));
		return 1;
	}

	if ((pwd = getpwuid(getuid())) == NULL) {
		(void) fprintf(stderr, "listusers: you don't exist\n");
		return 1;
	}

	asprintf(&userdn, "uid=%s,%s", pwd->pw_name, BASE_DN);
	
	(void) fprintf(stderr, "User DN: %s\n", userdn);

	if ((secret = get_ldap_secret()) == NULL)
		return 1;

	if ((err = ldap_simple_bind_s(conn, userdn, secret)) != 0) {
		(void) fprintf(stderr,
			"listusers: cannot bind as %s: %s\n",
			userdn, ldap_err2string(err));
		return 1;
	}

	memset(secret, 0, strlen(secret));

	if (argv[1])
		asprintf(&searchdn, "(&(objectClass=posixAccount)(uid=%s))", argv[1]);
	else
		asprintf(&searchdn, "(&(objectClass=posixAccount)(uid=*))");

	/*
	 * Print the user's existing email address.
	 */
	attrs[0] = "mail";
	attrs[1] = "uid";
	attrs[2] = "displayName";
	attrs[3] = NULL;
	err = ldap_search_s(conn, BASE_DN, LDAP_SCOPE_ONELEVEL, searchdn,
			attrs, 0, &result);
	if (err) {
		ldap_perror(conn,
			"listusers: retrieving user list");
		return 1;
	}

	if ((ent = ldap_first_entry(conn, result)) == NULL) {
		(void) fprintf(stderr,
			"listusers: no users found\n");
		return 1;
	}

	printf("%-16s %-30s %-40s\n", "Username", "Email", "Real name");
	printf("================ ============================== ========================================\n");

	do {
		if ((vals = ldap_get_values(conn, ent, "uid")) == NULL
		    || vals[0] == NULL)
			(void) printf("%-16s ", "(no uid)");
		else
			(void) printf("%-16s ", vals[0]);

		if ((vals = ldap_get_values(conn, ent, "mail")) == NULL
		    || vals[0] == NULL)
			(void) printf("%-30s ", "(no mail)");
		else
			(void) printf("%-30s ", vals[0]);

		if ((vals = ldap_get_values(conn, ent, "displayName")) == NULL
		    || vals[0] == NULL)
			(void) printf("%-40s ", "(no name)");
		else
			(void) printf("%-40s ", vals[0]);

		(void) printf("\n");
	} while ((ent = ldap_next_entry(conn, ent)) != NULL);

	return 0;
}
