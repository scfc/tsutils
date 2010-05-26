/*
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
 * Utilities for connecting to the LDAP server.
 */

#include	<stdio.h>
#include	<priv.h>
#include	<strings.h>
#include	<errno.h>
#include	<unistd.h>
#include	<pwd.h>

#include	<ldap.h>

#include	"tsutils.h"

#define SERVER          "ldap.toolserver.org"
#define PORT            LDAP_PORT
#define SECRET          "/etc/ldap_secret"
#define ADMIN_DN        "cn=Directory Manager"
#define BASE_DN         "ou=People,o=unix,o=toolserver"

static char ldap_secret[128];

const char *
get_ldap_secret()
{
FILE	*f;
size_t	 len;

	/* We require DAC_READ to open the secret file. */
	if (priv_set(PRIV_ON, PRIV_EFFECTIVE, PRIV_FILE_DAC_READ, NULL) == -1)
		return NULL;

	if ((f = fopen(SECRET, "r")) == NULL) {
		perror("cannot open LDAP secret");
		goto err;
	}

	if (priv_set(PRIV_OFF, PRIV_EFFECTIVE, PRIV_FILE_DAC_READ, NULL) == -1) {
		(void) fprintf(stderr, "get_ldap_secret: priv failure\n");
		_exit(1);
	}

	if (fgets(ldap_secret, sizeof ldap_secret, f) == NULL) {
		perror("cannot read LDAP secret");
		goto err;
	}

	(void) fclose(f);

	len = strlen(ldap_secret);
	if (len && ldap_secret[len - 1] == '\n')
		ldap_secret[len - 1] = '\0';

	return ldap_secret;

err:
	if (f)
		(void) fclose(f);
	return NULL;
}

void
clear_ldap_secret()
{
	bzero(ldap_secret, sizeof(ldap_secret));
}

LDAP *
ldap_connect_priv()
{
LDAP		*conn = NULL;
const char	*secret;
int		 err;

	if ((conn = ldap_init(SERVER, PORT)) == NULL) {
		(void) fprintf(stderr,
			"ldap_connect_priv: cannot connect to LDAP server: %s:%d: %s\n",
			SERVER, PORT, strerror(errno));
		return NULL;
	}

	if ((secret = get_ldap_secret()) == NULL)
		return NULL;

	if ((err = ldap_simple_bind_s(conn, ADMIN_DN, secret)) != 0) {
		(void) fprintf(stderr,
			"ldap_connect_priv: cannot bind as %s: %s\n",
			ADMIN_DN, ldap_err2string(err));
		goto err;
	}

	clear_ldap_secret();
	return conn;

err:
	clear_ldap_secret();
	return NULL;
}

LDAP *
ldap_connect()
{
LDAP		*conn = NULL;
int		 err;
char		*password;
struct passwd	*pwd;
char		*userdn = NULL;

	if ((pwd = getpwuid(getuid())) == NULL) {
		(void) fprintf(stderr, "ldap_connect: you don't seem to exist\n");
		goto err;
	}

	if ((conn = ldap_init(SERVER, PORT)) == NULL) {
		(void) fprintf(stderr,
			"ldap_connect: cannot connect to LDAP server: %s:%d: %s\n",
			SERVER, PORT, strerror(errno));
		goto err;
	}

	(void) asprintf(&userdn, "uid=%s,%s", pwd->pw_name, BASE_DN);

	(void) printf("User DN: %s\n", userdn);
	if ((password = ts_getpass("LDAP Password: ")) == NULL)
		goto err;

	if ((err = ldap_simple_bind_s(conn, userdn, password)) != 0) {
		(void) fprintf(stderr,
			"ldap_connect: cannot bind as %s: %s\n",
			userdn, ldap_err2string(err));
		goto err;
	}

	bzero(password, strlen(password));
	return conn;

err:
	if (password)
		bzero(password, strlen(password));
	free(userdn);
	return NULL;
}

int
ldap_user_replace_attr(conn, username, attr, value)
	LDAP		*conn;
	const char	*username, *attr, *value;
{
LDAPMod		 mod;
LDAPMod		*mods[2];
char		*attrs[2];
char		*userdn;
int		 err;

	(void) asprintf(&userdn, "uid=%s,%s", username, BASE_DN);
	bzero(&mod, sizeof(mod));

        mod.mod_op = LDAP_MOD_REPLACE;
        mod.mod_type = (char *) attr;
        attrs[0] = (char *) value;
	attrs[1] = NULL;

        mod.mod_values = attrs;

        mods[0] = &mod;
        mods[1] = NULL;

        if ((err = ldap_modify_s(conn, userdn, mods)) != 0) {
                ldap_perror(conn, "ldap_user_replace_attr");
		free(userdn);
                return -1;
        }

	free(userdn);
	return 0;
}

int
ldap_user_get_attr(conn, username, attr, value)
	LDAP		*conn;
	const char	*username, *attr;
	char		**value;
{
char		*userdn = NULL;
char		*attrs[2];
char		**vals;
int		 err;
LDAPMessage	*result, *ent;

	if (!conn || !username || !attr) {
		(void) fprintf(stderr, "ldap_user_get_attr: internal error\n");
		goto err;
	}

	(void) asprintf(&userdn, "uid=%s,%s", username, BASE_DN);

        attrs[0] = (char *) attr;
        attrs[1] = NULL;
        err = ldap_search_s(conn, userdn, LDAP_SCOPE_BASE,
                        "(objectclass=posixAccount)",
                        attrs, 0, &result);
        if (err) {
                ldap_perror(conn, "ldap_user_get_attr");
		goto err;
        }

        if ((ent = ldap_first_entry(conn, result)) == NULL) {
		*value = NULL;
		free(userdn);
		return 0;
        }

        if ((vals = ldap_get_values(conn, ent, attr)) == NULL
            || vals[0] == NULL) {
		*value = NULL;
		free(userdn);
		return 0;
	}

	*value = strdup(vals[0]);
	return 0;

err:
	free(userdn);
	return -1;
}
