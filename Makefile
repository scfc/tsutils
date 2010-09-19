VERSION	= $(shell date "+%Y%m%d_%H%M%S")
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
	setpass_fake	\
	getpasstofile	\
	summdisk	\
	setlicense	\
	resetpass	\
	resetpass_fake	\
	acctrenew	\
	acctrenew_fake
MAKEFLAGS	= --no-print-directory

include config.mk

all clean depend install lint::
	@for d in $(SUBDIRS); do \
		echo "$@ ==> $$d"; \
		$(MAKE) -C $$d $@ || exit 1; \
		echo "$@ <== $$d"; \
	done

clean::
	rm -f prototype pkginfo
	rm -rf stage
	rm -f TSutils.pkg
	rm -rf TSutils

prototype: prototype.in Makefile
	cat $<						\
		| sed -e "s,%prefix%,$(prefix),g"	\
		| sed -e "s,%PWD%,$$PWD,g"		\
		> $@
		
pkginfo: pkginfo.in Makefile
	cat $<						\
		| sed -e "s,%version%,$(VERSION),g"	\
		> $@

package: TSutils.pkg
TSutils.pkg: prototype pkginfo all
	$(MAKE) DESTDIR=$$PWD/stage install
	pkgmk -o -d $$PWD
	pkgtrans -o -s $$PWD $$PWD/TSutils.pkg TSutils

upload: package
	pfexec sh -c 'rm -rf /install/pkgs/custom/i386/TSutils && cp -r TSutils /install/pkgs/custom/i386'

.PHONY: package prototype pkginfo

