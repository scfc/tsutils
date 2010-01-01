SUBDIRS		= \
	libtsutils	\
	watcherd	\
	acctexp		\
	listlogins	\
	lsexp		\
	whodo		\
	whinequota	\
	days2date	\
	date2days	\
	readline	\
	setmail		\
       	setpass		\
	getpasstofile
MAKEFLAGS	= --no-print-directory

include config.mk

all clean depend install lint:
	@for d in $(SUBDIRS); do \
		echo "$@ ==> $$d"; \
		$(MAKE) -C $$d $@ || exit 1; \
		echo "$@ <== $$d"; \
	done

prototype: prototype.in Makefile
	cat $<						\
		| sed -e "s,%prefix%,$(prefix),g"	\
		| sed -e "s,%PWD%,$$PWD,g"		\
		> $@
		
pkginfo: pkginfo.in Makefile
	cat $<						\
		| sed -e "s,%version%,`svnversion`,g"	\
		> $@

package: TSutils.pkg
TSutils.pkg: prototype pkginfo all
	$(MAKE) DESTDIR=$$PWD/stage install
	pkgmk -o -d $$PWD
	pkgtrans -o -s $$PWD $$PWD/TSutils.pkg TSutils

.PHONY: package
