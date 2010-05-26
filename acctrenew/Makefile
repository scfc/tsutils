PROG		= acctrenew
BINMODE		= 4711

SRCS		= acctrenew.c
OBJS		= $(SRCS:.c=.o)
LIBS		= -lldap -lreadline

# No setuid tools on Linux
SKIP		= Linux
LIBTSUTILS	= YES

include ../mk/prog.mk
