if COMPILE_CMD
amulecmd_MANS = amulecmd.1
endif

if WEB
amuleweb_MANS = amuleweb.1
endif

if AMULE_GUI
amulegui_MANS = amulegui.1
endif

dist_man_MANS = $(amulecmd_MANS) $(amuleweb_MANS) $(amuleweb_MANS)

if MONOLITHIC
dist_man_MANS += amule.1
endif

if AMULE_DAEMON
dist_man_MANS += amuled.1
endif

if ED2K
dist_man_MANS += ed2k.1
endif

EXTRA_DIST = \
	amule.de.1 amuled.de.1 amulecmd.de.1 amulegui.de.1 amuleweb.de.1 ed2k.de.1 \
	amule.es.1 amuled.es.1 amulecmd.es.1 amuleweb.es.1 ed2k.es.1 \
	amule.eu.1 amuled.eu.1 amulecmd.eu.1 amuleweb.eu.1 ed2k.eu.1 \
	amule.fr.1 amuled.fr.1 amulecmd.fr.1 amuleweb.fr.1 ed2k.fr.1 \
	amule.hu.1 amuled.hu.1 amulecmd.hu.1 amuleweb.hu.1 ed2k.hu.1

man_langs = de es eu fr hu

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
