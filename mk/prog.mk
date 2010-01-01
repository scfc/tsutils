include ../mk/rules.mk

BINDIR	?= bin
bindir	= $(prefix)/$(BINDIR)
datadir	= $(prefix)/share
mandir	= $(datadir)/man/man$(MANSECT)
BINMODE	?= 755
OWNER	?= root
GROUP	?= bin
MANSECT	?= 1

ifeq ($(shell uname),SunOS)
INSTALL	?= ginstall
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
	$(INSTALL) -d -m 755 $(DESTDIR)$(bindir)
ifeq ($(DESTDIR),)
	$(INSTALL) -m $(BINMODE) -o $(OWNER) -g $(GROUP) $(PROG) $(bindir)
else
	$(INSTALL) $(PROG) $(DESTDIR)$(bindir)
endif
	if test -f $(PROG).$(MANSECT); then \
		$(INSTALL) -d -m 755 $(DESTDIR)$(mandir); \
		$(INSTALL) -m 644 $(PROG).$(MANSECT) $(DESTDIR)$(mandir); \
	fi

.PHONY: clean all install realinstall lint
