#LINT		= lint
#CC		= suncc
#CFLAGS		= -m64 -O -g -errwarn -xc99=all
#LINTFLAGS	= -axsm -u -errtags=yes -s -Xc99=%none -Xarch=amd64 -errsecurity=core -erroff=E_INCONS_ARG_DECL2

ifeq ($(shell uname),SunOS)
CC		= cc
CFLAGS		= -xO4 
else
CC		= gcc
CFLAGS		= -O2 -W -Wall -Werror
endif
