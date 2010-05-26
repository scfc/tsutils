/*
 *  Copyright (c) 2007, River Tarnell
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

#ifndef TSUTILS_H
#define TSUTILS_H

#include	<sys/types.h>
#include	<ldap.h>
#include	<priv.h>

char	*realloc_strcat(char *str, char const *add);
char	*realloc_strncat(char *str, char const *add, size_t len);
void	 strdup_free(char **s, char const *n);
int	 sendmail(char const *username, char const *message);
void	 logmsg(char const *msg, ...);
int	 daemon_detach(char const *progname);
int	 get_user_tty(char const *);
char	*file_to_string(char const *);
#if defined(__sun) && defined(__SVR4)
int	 asprintf(char **strp, char const *fmt, ...);
int	 daemon(int, int);
#endif

/* LDAP */

/* Return the LDAP secret for privileged connections */
char const *get_ldap_secret(void);
/* Clear the saved LDAP secret after use */
void clear_ldap_secret(void);

/* Connect to the LDAP server as the current user.  Prompts for a password. */
LDAP *ldap_connect(void);
/* Connect to the LDAP server as a privileged user */
LDAP *ldap_connect_priv(void);

/* Retrieve a single-value attribute from a user */
int ldap_user_get_attr(LDAP *conn, const char *username, const char *attr, char **result);
/* Set a single-value attribute for a user */
int ldap_user_replace_attr(LDAP *conn, const char *username, const char *attr, const char *value);

/* Read a passphrase from the user */
char *ts_getpass(const char *prompt);

#endif	/* !TSUTILS_H */
