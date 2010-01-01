/*
 * Read a password and output it to the specified file.
 * $Id$
 */

#include	<sys/fcntl.h>

#include	<unistd.h>
#include	<stdio.h>
#include	<errno.h>
#include	<string.h>
#include	<fcntl.h>
#include	<stdlib.h>

int
main(argc, argv)
	int argc;
	char **argv;
{
	int f;
	char *p;

	if (argc != 3) {
		(void) fprintf(stderr,
			"usage: %s <prompt> <file>\n",
			argv[0]);
		return 1;
	}

	if ((f = open(argv[2], O_CREAT | O_EXCL | O_RDWR, 0600)) == -1) {
		perror(argv[2]);
		return 1;
	}

#if defined(__sun) && defined(__SVR4)
	if ((p = getpassphrase(argv[1])) == NULL) {
#else
	if ((p = getpass(argv[1])) == NULL) {
#endif
		perror("getpass");
		return 1;
	}

	write(f, p, strlen(p));
	return 0;
}
