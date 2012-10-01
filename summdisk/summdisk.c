/* Copyright (c) 2010 River Tarnell <river@loreley.flyingparchment.org.uk>. */
/* Copyright (c) 2012 Tim Landscheidt <tim@tim-landscheidt.de>. */
/*
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely. This software is provided 'as-is', without any express or implied
 * warranty.
 */
 
/* $Id$ */
 
/*
 * summdisk: summarise disk usage in a directory by user id
 */

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<inttypes.h>
#include	<dirent.h>
#include	<stdlib.h>
#include	<string.h>
#include	<pwd.h>

typedef struct uent {
	uid_t	ue_user;
	int64_t	ue_nbytes;
	int	ue_nfiles;
} uent_t;

uent_t *users;
size_t nusers;

static uent_t *getuser(uid_t);

static void summarise(int);
static void report(void);
static int usercmp(const void *, const void *);

int
main(argc, argv)
	int	  argc;
	char	**argv;
{
int		 dir;
char const	*path = ".";

	if (argc > 1)
		path = argv[1];
	if ((dir = open64(path, O_RDONLY)) == NULL) {
		perror(path);
		return 1;
	}
	summarise(dir);
	(void) close(dir);
	report();
	return 0;
}

static uent_t *
getuser(uid)
	uid_t	uid;
{
size_t	i = 0;
	for (; i < nusers; ++i) {
		if (users[i].ue_user == uid)
			return &users[i];
	}

	if ((users = realloc(users, sizeof(uent_t) * (nusers + 1))) == NULL) {
		(void) fprintf(stderr, "out of memory\n");
		exit(1);
	}

	users[nusers].ue_user = uid;
	users[nusers].ue_nbytes = 0;
	nusers++;
	return &users[nusers - 1];
}

void
summarise(dfd)
	int	dfd;
{
DIR		*dir;
struct dirent	*de;
struct stat64	 sb;
int		 ffd;
	if ((dir = fdopendir(dfd)) == NULL) {
		perror("fdopendir");
		exit(1);
	}

	while ((de = readdir(dir)) != NULL) {
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;

		if (fstatat64(dfd, de->d_name, &sb, AT_SYMLINK_NOFOLLOW) == -1) {
			perror(de->d_name);
			exit(1);
		}

		if (S_ISDIR(sb.st_mode)) {
			if ((ffd = openat64(dfd, de->d_name, O_RDONLY)) == -1) {
				perror(de->d_name);
				exit(1);
			}

			summarise(ffd);
			(void) close(ffd);
		} else if (S_ISREG(sb.st_mode)) {
		uent_t	*user = getuser(sb.st_uid);
			user->ue_nbytes += sb.st_size;
			user->ue_nfiles++;
		}
	}

	(void) closedir(dir);
}

static int
usercmp(a, b)
	const void	*a, *b;
{
	if (((uent_t *) a)->ue_nbytes > ((uent_t *) b)->ue_nbytes)
		return -1;
	else if (((uent_t *) a)->ue_nbytes < ((uent_t *) b)->ue_nbytes)
		return 1;
	else
		return 0;
}

static const char *
humanise(n)
	uint64_t	n;
{
static char const names[] = "bKMGTPEZY";
static char res[32];
double	d = n;
int	i = 0;
	while (d > 1024) {
		i++;
		d /= 1024;
	}
	(void) snprintf(res, sizeof res, "%.2lf%c", d, names[i]);
	return res;
}

static void
report()
{
size_t	i = 0;
	qsort(users, nusers, sizeof(*users), usercmp);
	for (; i < nusers; ++i) {
	struct passwd	*pwd;
	char		 uname[32];
		if (pwd = getpwuid(users[i].ue_user))
			strlcpy(uname, pwd->pw_name, sizeof uname);
		else
			snprintf(uname, sizeof uname, "%d", (int) users[i].ue_user);
		printf("%-16s : %7s in %5d files\n",
			uname, humanise(users[i].ue_nbytes), users[i].ue_nfiles);
	}
}

