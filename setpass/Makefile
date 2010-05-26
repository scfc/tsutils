PROG		= setpass
BINMODE		= 4711
GROUP		= root

SRCS		= setpass.c
OBJS		= $(SRCS:.c=.o)
LIBS		= -lldap

# No setuid tools on Linux
SKIP		= Linux
LIBTSUTILS	= YES

include ../mk/prog.mk
