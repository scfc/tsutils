include ../mk/rules.mk

BINDIR	?= /usr/local/bin
MANDIR	?= /usr/local/man/man$(MANSECT)
BINMODE	?= 755
OWNER	?= root
GROUP	?= bin
MANSECT	?= 1

ifeq ($(shell uname),SunOS)
INSTALL	?= /opt/ts/gnu/bin/install
else
INSTALL	?= install
endif

ifeq ($(findstring $(shell uname),$(SKIP)),$(shell uname))
all: 
install:
else
all: $(PROG)
install: all realinstall $(INSTALLEXTRA)
endif

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)
lint:
	$(LINT) $(LINTFLAGS) $(INCLUDES) $(SRCS) $(LIBS)
clean:
	rm -f $(PROG) $(OBJS)

realinstall:
	$(INSTALL) -d -m 755 $(BINDIR)
	$(INSTALL) -m $(BINMODE) -o $(OWNER) -g $(GROUP) $(PROG) $(BINDIR)
	if test -f $(PROG).$(MANSECT); then \
		$(INSTALL) -d -m 755 $(MANDIR); \
		$(INSTALL) -m 644 $(PROG).$(MANSECT) $(MANDIR); \
	fi

.PHONY: clean all install realinstall lint
