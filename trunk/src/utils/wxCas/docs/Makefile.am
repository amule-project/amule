dist_man_MANS = @WXCAS_MANS@

EXTRA_DIST = wxcas.1 wxcas.de.1 wxcas.es.1 wxcas.eu.1 wxcas.hu.1

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
