include ../config.mk

ifeq ($(LIBTSUTILS),YES)
INCLUDES	+= -I../libtsutils
LIBS		+= -L../libtsutils -ltsutils
endif

ifeq ($(MYSQL),YES)
INCLUDES	+= $(shell mysql_config --include)
LIBS		+= $(shell mysql_config --libs)
endif

ifeq ($(shell uname),SunOS)
INCLUDES	+= -I/opt/ts/include
LDFLAGS		+= -L/opt/ts/lib -R/opt/ts/lib
endif

.c.o:
	$(CC) $(CPPFLAGS) $(INCLUDES) $(CFLAGS) -c $<
