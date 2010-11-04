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

#include	<fcntl.h>
#include	<termios.h>
#include	<strings.h>
#include	<unistd.h>
#include	<stdio.h>

char *
ts_getpass(prompt)
	const char	*prompt;
{
int		tt;
char		c;
#define MAXPASS	128
static char	pass[MAXPASS + 1];
int		n = 0, i = 0;
struct termios	nt, ot;

	if (prompt)
		(void) fputs(prompt, stdout);

	if ((tt = open("/dev/tty", O_RDWR)) == -1)
		return NULL;

	if (tcgetattr(tt, &ot) == -1)
		return NULL;
	bcopy(&ot, &nt, sizeof(nt));
	nt.c_lflag &= ~(ICANON | ECHO);
	nt.c_cc[VMIN] = 1;
	nt.c_cc[VTIME] = 0;
	if (tcsetattr(tt, TCSAFLUSH, &nt) == -1)
		return NULL;

	bzero(pass, sizeof (pass));

	while (read(tt, &c, 1) == 1) {
		switch (c) {
		case '\r':
		case '\n':

			for (i = 0; i < n; ++i)
				(void) putc('\b', stdout);
			for (i = 0; i < n; ++i)
				(void) putc(' ', stdout);
			(void) tcsetattr(tt, TCSAFLUSH, &ot);
			(void) close(tt);
			(void) putc('\n', stdout);
			(void) fflush(stdout);
			return pass;

		case 0x7F:	/* DEL */
		case 0x08:	/* ^H */
			if (n == 0) {
				(void) write(tt, "\a", 1);
				break;
			}

			pass[--n] = 0;
			(void) write(tt, "\b \b", 3);
			break;

		default:
			if (n == MAXPASS) {
				(void) write(tt, "\a", 1);
				break;
			}

			pass[n++] = c;
			(void) write(tt, "*", 1);
			break;
		}
	}

	(void) tcsetattr(tt, TCSAFLUSH, &ot);
	(void) close(tt);
	return NULL;
}
