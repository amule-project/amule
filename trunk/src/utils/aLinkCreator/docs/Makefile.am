dist_man_MANS = @ALC_MANS@ @ALCC_MANS@

EXTRA_DIST = \
	alc.1 alcc.1 \
	alc.de.1 alcc.de.1 \
	alc.es.1 alcc.es.1 \
	alc.eu.1 alcc.eu.1 \
	alcc.fr.1 \
	alc.hu.1 alcc.hu.1

install-data-hook:
	@for file in ${dist_man_MANS}; do \
		lang=`echo $$file |  sed -e 's/^[^.]*\.//'`; \
		if test $$lang == 1; then \
			echo " $(mkinstalldirs) $(DESTDIR)$(mandir)/man1" ; \
			$(mkinstalldirs) $(DESTDIR)$(mandir)/man1 ; \
			echo " $(INSTALL_DATA) $$file $(DESTDIR)$(mandir)/man1/$$file" ; \
			$(INSTALL_DATA) $$file $(DESTDIR)$(mandir)/man1/$$file ; \
		else \
			locale=`echo $$lang | sed -e 's/\..*//'` ; \
			inst=`echo $$file | sed -e 's/\..*\././'`; \
			echo " $(mkinstalldirs) $(DESTDIR)$(mandir)/$$locale/man1" ; \
			$(mkinstalldirs) $(DESTDIR)$(mandir)/$$locale/man1 ; \
			echo " $(INSTALL_DATA) $$file $(DESTDIR)$(mandir)/$$locale/man1/$$inst" ; \
			$(INSTALL_DATA) $$file $(DESTDIR)$(mandir)/$$locale/man1/$$inst ; \
		fi; \
	done

uninstall-hook:
	@for file in ${dist_man_MANS}; do \
		lang=`echo $$file |  sed -e 's/^[^.]*\.//'`; \
		if test $$lang == 1; then \
			rm -f $(DESTDIR)$(mandir)/man1/$$file ; \
		else \
			locale=`echo $$lang | sed -e 's/\..*//'` ; \
			inst=`echo $$file | sed -e 's/\..*\././'`; \
			rm -f $(DESTDIR)$(mandir)/$$locale/man1/$$inst ; \
		fi; \
	done

MAINTAINERCLEANFILES = Makefile.in
