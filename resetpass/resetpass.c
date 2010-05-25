/* Copyright (c) 2008, 2010 River Tarnell <river@wikimedia.org>. */
/*
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely. This software is provided 'as-is', without any express or implied
 * warranty.
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

#include	<ldap.h>
#include	<readline/readline.h>

#define SERVER		"ldap.toolserver.org"
#define PORT		LDAP_PORT
#define SECRET		"/etc/ldap_secret"
#define ADMIN_DN 	"cn=Directory Manager"
#define BASE_DN		"ou=People,o=unix,o=toolserver"

#if defined(__sun) && defined(__SVR4)
# define getpass getpassphrase
#endif

#if defined(__sun) && defined(__SVR4)
int asprintf(char **, char const *, ...);
#endif
static const char *mktoken(void);
static const char *get_ldap_secret(void);
static int check_utmp(const char *username);
static int set_password(LDAP *conn, const char *userdn);
static int validate_token(const char *username, const char *email);
static int generate_token(const char *username, const char *email);

#define TOKENDIR "/var/resetpass"
#define TOKENSZ 32 /* Maximum length, not actual length */
#define TOKENCHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

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

		
		
#if defined(__sun) && defined(__SVR4)
int
asprintf(char **strp, char const *fmt, ...)
{
va_list ap;
int     len;
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

static const char *
get_ldap_secret(void)
{
FILE		*f;
static char	 buf[128];
size_t		 len;

	if ((f = fopen(SECRET, "r")) == NULL) {
		(void) fprintf(stderr,
			"setpass: cannot open LDAP secret: %s\n",
			strerror(errno));
		return NULL;
	}

	if (fgets(buf, sizeof buf, f) == NULL) {
		(void) fprintf(stderr,
			"setpass: cannot read LDAP secret: %s\n",
			strerror(errno));
		return NULL;
	}

	fclose(f);

	len = strlen(buf);
	if (len && buf[len - 1] == '\n')
		buf[len - 1] = '\0';

	return buf;
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
	char **argv
#ifdef __GNUC__
	__attribute__((unused))
#endif
	;
{
LDAP		*conn;
char		*userdn;
const char	*secret;
struct passwd	*pwd;
int		 err;
char		*attrs[2];
LDAPMessage	*result, *ent;
char		**vals;
char		*email;

	/*
	 * At this point we're running as root.  This is necessary to create
	 * the token directory.
	 */
	umask(07);

	if (mkdir(TOKENDIR, 0770) == -1 && errno != EEXIST) {
		perror("resetpass: internal error");
		return 1;
	}

	/* Drop privs */
	if (seteuid(getuid()) == -1) {
		perror("resetpass: internal error (setuid)");
		return 1;
	}

	/* Now we're running setgid root */

	if (argc != 1) {
		(void) fprintf(stderr, "usage: resetpass\n");
		return 1;
	}

	if ((conn = ldap_init(SERVER, PORT)) == NULL) {
		(void) fprintf(stderr,
			"resetpass: cannot connect to LDAP server: %s:%d: %s\n",
			SERVER, PORT, strerror(errno));
		return 1;
	}

	if ((secret = get_ldap_secret()) == NULL)
		return 1;

	if ((err = ldap_simple_bind_s(conn, ADMIN_DN, secret)) != 0) {
		(void) fprintf(stderr,
			"resetpass: cannot bind as %s: %s\n",
			ADMIN_DN, ldap_err2string(err));
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

	asprintf(&userdn, "uid=%s,%s", pwd->pw_name, BASE_DN);

	/*
	 * If the user's password is not set, don't allow it to be reset.
	 */
	attrs[0] = "userPassword";
	attrs[1] = NULL;
	err = ldap_search_s(conn, userdn, LDAP_SCOPE_BASE,
			"(objectclass=posixAccount)",
			attrs, 0, &result);
	if (err) {
		ldap_perror(conn,
			"resetpass: retrieving current userPassword");
		return 1;
	}

	if ((ent = ldap_first_entry(conn, result)) == NULL) {
		(void) fprintf(stderr,
			"resetpass: no result when looking for current userPassword\n");
		return 1;
	}

	if ((vals = ldap_get_values(conn, ent, "userPassword")) == NULL
	    || vals[0] == NULL) {
		(void) fprintf(stderr,
			"resetpass: object has no userPassword\n");
		return 1;
	}

	if (!strcmp(vals[0], "{crypt}!")) {
		(void) fprintf(stderr, "resetpass: password not set\n");
		(void) fprintf(stderr, "resetpass: use setpass(1) to set your password\n");
		return 1;
	}

	/*
	 * Fetch the user's email address.
	 */
	attrs[0] = "mail";
	attrs[1] = NULL;
	err = ldap_search_s(conn, userdn, LDAP_SCOPE_BASE,
			"(objectclass=posixAccount)",
			attrs, 0, &result);
	if (err) {
		ldap_perror(conn,
			"resetpass: retrieving email address");
		return 1;
	}

	if ((ent = ldap_first_entry(conn, result)) == NULL) {
		(void) fprintf(stderr,
			"resetpass: no result when looking for email address\n");
		return 1;
	}

	if ((vals = ldap_get_values(conn, ent, "mail")) == NULL
	    || vals[0] == NULL) {
		(void) fprintf(stderr,
			"resetpass: you don't have an email address set.\n");
		(void) fprintf(stderr,
			"resetpass: use setmail(1) before running this program.\n");
		return 1;
	}
	email = strdup(vals[0]);

	if (!validate_token(pwd->pw_name, email))
		return 0;
	else
		if (set_password(conn, userdn) != 0) {
			(void) fprintf(stderr, "resetpass: re-run to try again\n");
			return 1;
		} else {
			/* Remove the token */
		int	d;
			if ((d = open(TOKENDIR, O_RDONLY)) == -1) {
				perror("resetpass: internal error (remove token)");
				return 1;
			}

			unlinkat(d, pwd->pw_name, 0);
		}

	return 0;
}

static int
set_password(conn, userdn)
	LDAP		*conn;
	const char	*userdn;
{
char		*attrs[2];
LDAPMod		 mod;
LDAPMod		*mods[2];
char		 newpass[128], verify[128], *s;
int		 err;

	if ((s = getpass("Enter new password: ")) == NULL)
		return 1;
	strncpy(newpass, s, sizeof(newpass));
	newpass[sizeof(newpass) - 1] = 0;

	if ((s = getpass("Re-enter password: ")) == NULL)
		return 1;
	strncpy(verify, s, sizeof(verify));
	verify[sizeof(verify) - 1] = 0;

	if (strcmp(newpass, verify)) {
		(void) fprintf(stderr, "resetpass: passwords don't match\n");
		return 1;
	}

	memset(&mod, 0, sizeof(mod));
	mod.mod_op = LDAP_MOD_REPLACE;
	mod.mod_type = "userPassword";
	attrs[0] = newpass;
	attrs[1] = NULL;
	mod.mod_values = attrs;

	mods[0] = &mod;
	mods[1] = NULL;

	if ((err = ldap_modify_s(conn, userdn, mods)) != 0) {
		ldap_perror(conn, "resetpass: setting new password");
		return 1;
	}

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

	if ((d = open(TOKENDIR, O_RDONLY)) == -1) {
		perror("resetpass: internal error (open tokendir)");
		goto err;
	}

	if ((f = openat(d, username, O_RDONLY)) == -1) {
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
			perror("resetpass: internal error (fstat)");
			goto err;
		}

		if (read(f, token, TOKENSZ) < 0) {
			perror("resetpass: internal error (read)");
			goto err;
		}

		tm = gmtime(&sb.st_mtime);
		strftime(tfmt, sizeof (tfmt), "%d-%b-%Y %H:%M:%S", tm);
		(void) printf(
"\nA password reset token was emailed to you on %s.\n"
"If you did not receive this token, press enter to generate a new token.\n\n",
			tfmt
		);

		if ((try = readline("Password reset token: ")) == NULL)
			goto err;

		printf("\n");

		if (!*try) {
			unlinkat(d, username, 0);
			if (generate_token(username, email) < 0)
				(void) fprintf(stderr, "resetpass: internal error sending token\n");
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
int		 status;

	if ((d = open(TOKENDIR, O_RDONLY)) == -1) {
		perror("resetpass: internal error (open token directory)");
		goto err;
	}

	if ((f = openat(d, username, O_WRONLY | O_CREAT | O_EXCL, 0770)) == -1) {
		perror("resetpass: internal error (write token");
		goto err;
	}

	if ((ntok = mktoken()) == NULL) {
		perror("resetpass: internal error (create token)");
		goto err;
	}

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
	if (seteuid(0) == -1) {
		perror("resetpass: internal error (seteuid)");
		_exit(1);
	}

	if (setuid(0) == -1) {
		perror("resetpass: internal error (setuid)");
		_exit(1);
	}

	switch (fork()) {
	case 0: { /* child */
	char const * const env[] = {
		"PATH=/usr/bin",
		NULL
	};
	char const * const args[] = {
		"/usr/lib/sendmail",
		"-oi", "-bm", "--", username,
		NULL
	};
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
	if (setuid(olduid) == -1) {
		perror("resetpass: internal error (setuid)");
		_exit(1);
	}

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
