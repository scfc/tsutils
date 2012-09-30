man_MANS = acctrenew.1

if TSUTILS_LINUX
bin_SCRIPTS = acctrenew

acctrenew: ../fake_wrapper
	cp $< $@
else
bin_PROGRAMS = acctrenew

acctrenew_CFLAGS = -I../libtsutils
acctrenew_LDADD = -lldap -lreadline -L../libtsutils -ltsutils

install-exec-hook:
	-chown root.root $(DESTDIR)$(bindir)/acctrenew
	-chmod 4711 $(DESTDIR)$(bindir)/acctrenew
endif
