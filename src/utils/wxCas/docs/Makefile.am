dist_man_MANS = wxcas.1

man_langs = de es eu hu

EXTRA_DIST = wxcas.de.1 wxcas.es.1 wxcas.eu.1 wxcas.hu.1

install-data-hook:
	@for lang in $(man_langs); do \
		echo " $(mkinstalldirs) $(DESTDIR)$(mandir)/$$lang/man1" ; \
		$(mkinstalldirs) $(DESTDIR)$(mandir)/$$lang/man1 ; \
		list='$(dist_man_MANS)' ; \
		for file in $$list; do \
			tmp=`echo $$file | sed -e "s/\.1/\.$$lang\.1/"` ; \
			if test -f $(srcdir)/$$tmp; then name=$(srcdir)/$$tmp; \
			else name=$$tmp; fi; \
			inst=`echo $$file | sed -e "s/\.1//" -e '$(transform)'`.1 ; \
			echo " $(INSTALL_DATA) $$name $(DESTDIR)$(mandir)/$$lang/man1/$$inst" ; \
			$(INSTALL_DATA) $$name $(DESTDIR)$(mandir)/$$lang/man1/$$inst ; \
		done ; \
	done

uninstall-hook:
	@for lang in $(man_langs); do \
		list='$(dist_man_MANS)' ; \
		for file in $$list; do \
			inst=`echo $$file | sed -e "s/\.1//" -e '$(transform)'`.1 ; \
			echo " rm -f $(DESTDIR)$(mandir)/$$lang/man1/$$inst" ; \
			rm -f $(DESTDIR)$(mandir)/$$lang/man1/$$inst ; \
		done ; \
	done

MAINTAINERCLEANFILES = Makefile.in
