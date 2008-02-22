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

EXTRA_DIST = \
	amule.1 amuled.1 amulecmd.1 amulegui.1 amuleweb.1 ed2k.1 \
	amule.de.1 amuled.de.1 amulecmd.de.1 amulegui.de.1 amuleweb.de.1 ed2k.de.1 \
	amule.es.1 amuled.es.1 amulecmd.es.1 amuleweb.es.1 ed2k.es.1 \
	amule.eu.1 amuled.eu.1 amulecmd.eu.1 amuleweb.eu.1 ed2k.eu.1 \
	amule.fr.1 amuled.fr.1 amulecmd.fr.1 amuleweb.fr.1 ed2k.fr.1 \
	amule.hu.1 amuled.hu.1 amulecmd.hu.1 amuleweb.hu.1 ed2k.hu.1

EXTRA_DIST += \
	manpages.pot \
	manpages-de.po

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
