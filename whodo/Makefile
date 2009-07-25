PROG		= whodo
SRCS		= whodo.c
OBJS		= $(SRCS:.c=.o)

# Not supported on FreeBSD (no utmpx)
# Solaris already has whodo.
SKIP		= FreeBSD SunOS

include ../mk/prog.mk
