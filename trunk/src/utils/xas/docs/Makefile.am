dist_man_MANS = xas.1

man_langs = de es eu hu

EXTRA_DIST = xas.de.1 xas.es.1 xas.eu.1 xas.hu.1

install-data-hook:
	@for lang in $(man_langs); do \
		echo " $(mkinstalldirs) $(DESTDIR)$(mandir)/$$lang/man1" ; \
		$(mkinstalldirs) $(DESTDIR)$(mandir)/$$lang/man1 ; \
		list='$(dist_man_MANS)' ; \
		for file in $$list; do \
			tmp=`echo $$file | sed -e "s/\.1/\.$$lang\.1/"` ; \
			if test -f $(srcdir)/$$tmp; then name=$(srcdir)/$$tmp; \
			else name=$$tmp; fi; \
			echo " $(INSTALL_DATA) $$name $(DESTDIR)$(mandir)/$$lang/man1/$$file" ; \
			$(INSTALL_DATA) $$name $(DESTDIR)$(mandir)/$$lang/man1/$$file ; \
		done ; \
	done

uninstall-hook:
	@for lang in $(man_langs); do \
		list='$(dist_man_MANS)' ; \
		for file in $$list; do \
			name=`echo $$file | sed -e "s/\.1/\.$$lang\.1/"` ; \
			echo " rm -f $(DESTDIR)$(mandir)/$$lang/man1/$$file" ; \
			rm -f $(DESTDIR)$(mandir)/$$lang/man1/$$file ; \
		done ; \
	done
