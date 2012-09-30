man_MANS = setpass.1

if TSUTILS_LINUX
bin_SCRIPTS = setpass

setpass: ../fake_wrapper
	cp $< $@
else
bin_PROGRAMS = setpass

setpass_CFLAGS = -I../libtsutils
setpass_LDADD = -lldap -L../libtsutils -ltsutils

install-exec-hook:
	-chown root.root $(DESTDIR)$(bindir)/setpass
	-chmod 4711 $(DESTDIR)$(bindir)/setpass
endif
