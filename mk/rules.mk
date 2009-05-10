include ../config.mk

ifeq ($(LIBTSUTILS),YES)
INCLUDES	+= -I../libtsutils
LIBS		+= -L../libtsutils -ltsutils
endif

ifeq ($(MYSQL),YES)
INCLUDES	+= $(shell mysql_config --include)
LIBS		+= $(shell mysql_config --libs)
endif

.c.o:
	$(CC) $(CPPFLAGS) $(INCLUDES) $(CFLAGS) -c $<
