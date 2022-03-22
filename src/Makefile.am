appdefaultdir=$(shell pkg-config --variable=appdefaultdir xt)
datarootdir=$(shell pkg-config --variable=datarootdir xt)

SUBDIRS = .
bin_PROGRAMS = xvkbd
man_MANS = xvkbd.1

xvkbd_SOURCES = xvkbd.c findwidget.c resources.h XVkbd-common.h
xvkbd_LDADD = @XAW_LIBS@ @X11_LIBS@
xvkbd_CFLAGS = @XAW_CFLAGS@ @X11_CFLAGS@ -DUSE_I18N -DUSE_XTEST -Wall -DSHAREDIR=\"$(datarootdir)\/xvkbd\"

xvkbd.1: xvkbd.man
	[ ! -e $@ ] ||  rm $@
	-ln -s $< $@

XVkbd-common.h: XVkbd-common.ad
	[ which ad2c > /dev/null ] || ad2c XVkbd-common.ad > XVkbd-common.h

install-data-hook:
	[ -d $(DESTDIR)$(appdefaultdir) ] || mkdir $(DESTDIR)$(appdefaultdir)
	for file in *.ad; do \
		$(INSTALL_DATA) $$file $(DESTDIR)$(appdefaultdir)/$${file%.ad}; \
	done
	[ -d $(DESTDIR)$(datarootdir)/xvkbd/ ] || mkdir $(DESTDIR)$(datarootdir)/xvkbd/
	$(INSTALL_DATA) words.english $(DESTDIR)$(datarootdir)/xvkbd/words.english

EXTRA_DIST = XVkbd-common.h *.ad *.xbm xvkbd.man README ChangeLog Imakefile words.english

clean::
	rm -f *~ *.bak

distclean::
	make clean
	rm -rf autom4te.cache *.o *.log Makefile

README:
	make doc

doc:
	[ -f Makefile.docs ] && make -f Makefile.docs

distclean::
	rm -f xvkbd-*.tar.gz
	[ -f Makefile.docs ] && make -f Makefile.docs cleandir

