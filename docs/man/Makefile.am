if COMPILE_CMD
amulecmd_MANS = @AMULECMD_MANS@
endif

if WEB
amuleweb_MANS = @WEB_MANS@
endif

if AMULE_GUI
amulegui_MANS = @AMULE_GUI_MANS@
endif

if MONOLITHIC
monolithic_MANS = @MONOLITHIC_MANS@
endif

if AMULE_DAEMON
daemon_MANS = @DAEMON_MANS@
endif

if ED2K
ed2k_MANS = @ED2K_MANS@
endif

dist_man_MANS = $(amulecmd_MANS) $(amuleweb_MANS) $(amuleweb_MANS) $(monolithic_MANS) $(daemon_MANS) $(ed2k_MANS) $(amulegui_MANS)

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
			rm -f $$file $(DESTDIR)$(mandir)/$$locale/man1/$$inst ; \
		fi; \
	done

MAINTAINERCLEANFILES = Makefile.in
