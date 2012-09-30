man_MANS = resetpass.1

if TSUTILS_LINUX
bin_SCRIPTS = resetpass

resetpass: ../fake_wrapper
	cp $< $@
else
bin_PROGRAMS = resetpass
resetpass_CFLAGS = -I../libtsutils
resetpass_LDADD = -lldap -L../libtsutils -ltsutils

install-exec-hook:
	-chown root.root $(DESTDIR)$(bindir)/resetpass
	-chmod 4711 $(DESTDIR)$(bindir)/resetpass
endif
