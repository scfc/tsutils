PROG		= resetpass
BINMODE		= 4711
GROUP		= root

SRCS		= resetpass.c
OBJS		= $(SRCS:.c=.o)
LIBS		= -lldap -lreadline

# No setuid tools on Linux
SKIP		= Linux
LIBTSUTILS	= YES

include ../mk/prog.mk
