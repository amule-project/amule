dlg_MANS =

if COMPILE_CMD
amulecmd_MANS = amulecmd.1
endif

if GUI
amulecmd_MANS = amulecmd.1
dlg_MANS += amulecmd
endif

if WEB
amuleweb_MANS = amuleweb.1
endif

if WEBGUI
amuleweb_MANS = amuleweb.1
dlg_MANS += amuleweb
endif

dist_man_MANS = $(amulecmd_MANS) $(amuleweb_MANS)

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
	amule.de.1 amuled.de.1 amulecmd.de.1 amuleweb.de.1 ed2k.de.1 \
	amule.es.1 amuled.es.1 amulecmd.es.1 amuleweb.es.1 ed2k.es.1 \
	amule.fr.1 amuled.fr.1 amulecmd.fr.1 amuleweb.fr.1 ed2k.fr.1 \
	amule.hu.1 amuled.hu.1 amulecmd.hu.1 amuleweb.hu.1 ed2k.hu.1

man_langs = de es fr hu

install-data-hook:
	@list='$(dlg_MANS)'; \
	for file in $$list; do \
		inst=`echo $${file}DLG | sed '$(transform)'`.1 ; \
		echo " $(INSTALL_DATA) $${file}DLG.1 $(DESTDIR)$(man1dir)/$$inst" ; \
		ref=`echo $$file | sed '$(transform)'`.1 ; \
		echo ".so $(man1dir)/$$ref" > $${file}DLG.1 ; \
		$(INSTALL_DATA) $${file}DLG.1 $(DESTDIR)$(man1dir)/$$inst ; \
		rm -f $${file}DLG.1 ; \
	done
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
		list='$(dlg_MANS)'; \
		for file in $$list; do \
			inst=`echo $${file}DLG | sed '$(transform)'`.1 ; \
			echo " $(INSTALL_DATA) $(mandir)/$$lang/man1/$${file}DLG.$$lang.1 $(DESTDIR)$(mandir)/$$lang/man1/$$inst" ; \
			ref=`echo $$file | sed '$(transform)'`.1 ; \
			echo ".so $(mandir)/$$lang/man1/$$ref" > $${file}DLG.$$lang.1 ; \
			$(INSTALL_DATA) $${file}DLG.$$lang.1 $(DESTDIR)$(mandir)/$$lang/man1/$$inst ; \
			rm -f $${file}DLG.$$lang.1 ; \
		done ; \
	done

uninstall-hook:
	@list='$(dlg_MANS)'; \
	for file in $$list; do \
		inst=`echo $${file}DLG | sed '$(transform)'`.1 ; \
		echo " rm -f $(DESTDIR)$(man1dir)/$$inst" ; \
		rm -f $(DESTDIR)$(man1dir)/$$inst ; \
	done
	@for lang in $(man_langs); do \
		list='$(dist_man_MANS)' ; \
		for file in $$list; do \
			inst=`echo $$file | sed -e "s/\.1//" -e '$(transform)'`.1 ; \
			echo " rm -f $(DESTDIR)$(mandir)/$$lang/man1/$$inst" ; \
			rm -f $(DESTDIR)$(mandir)/$$lang/man1/$$inst ; \
		done ; \
		list='$(dlg_MANS)'; \
		for file in $$list; do \
			inst=`echo $${file}DLG | sed '$(transform)'`.1 ; \
			echo " rm -f $(DESTDIR)$(mandir)/$$lang/man1/$$inst" ; \
			rm -f $(DESTDIR)$(mandir)/$$lang/man1/$$inst ; \
		done ; \
	done
