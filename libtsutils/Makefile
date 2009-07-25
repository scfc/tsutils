LIB	= tsutils
SRCS	= tsutils.c mail.c write.c subst.c
OBJS	= $(SRCS:.c=.o)

ifeq ($(shell uname),SunOS)
SRCS	+= daemon.c
OBJS	+= daemon.o
endif

include ../mk/lib.mk
