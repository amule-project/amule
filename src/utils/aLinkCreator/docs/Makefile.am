dist_man_MANS =

if ALC
dist_man_MANS += alc.1
endif

if ALCC
dist_man_MANS += alcc.1
endif

man_langs = de es eu fr hu

EXTRA_DIST = \
	alc.de.1 alcc.de.1 \
	alc.es.1 alcc.es.1 \
	alc.eu.1 alcc.eu.1 \
	alcc.fr.1 \
	alc.hu.1 alcc.hu.1

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
