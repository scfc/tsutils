ACLOCAL_AMFLAGS = -I m4

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
	getpasstofile	\
	summdisk	\
	setlicense	\
	resetpass	\
	acctrenew	\
	passwd
MAKEFLAGS	= --no-print-directory

#LINT		= lint
#CC		= suncc
#CFLAGS		= -m64 -O -g -errwarn -xc99=all
LINTFLAGS	= -axsm -u -errtags=yes -s -Xc99=%none -errsecurity=core -erroff=E_INCONS_ARG_DECL2

# ifeq ($TARGET_OS,SunOS)
# CC		= cc
# CFLAGS		= -xO4 
# prefix		= /opt/local
# else
# CC		= gcc
# CFLAGS		= -O2 -W -Wall -Werror
# prefix=/usr/local
# endif
# ifeq ($(TARGET_OS),SunOS)
# INCLUDES	+= -I/opt/ts/include
# LDFLAGS		+= -L/opt/ts/lib -R/opt/ts/lib
# endif

# clean::
# 	rm -f prototype pkginfo
# 	rm -rf stage
# 	rm -f TSutils.pkg
# 	rm -rf TSutils

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
