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

EXTRA_DIST = amule.de.1 amuled.de.1 amulecmd.de.1 amuleweb.de.1 ed2k.de.1

man_langs = de

install-data-hook:
	@list='$(dlg_MANS)'; \
	for file in $$list; do \
		echo " rm -f $(DESTDIR)$(man1dir)/$${file}DLG.1" ; \
		rm -f $(DESTDIR)$(man1dir)/$${file}DLG.1 ; \
		echo " $(LN_S) $(man1dir)/$${file}.1 $(DESTDIR)$(man1dir)/$${file}DLG.1" ; \
		$(LN_S) $(man1dir)/$${file}.1 $(DESTDIR)$(man1dir)/$${file}DLG.1 ; \
	done
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
		list='$(dlg_MANS)'; \
		for file in $$list; do \
			echo " rm -f $(DESTDIR)$(mandir)/$$lang/man1/$${file}DLG.1" ; \
			rm -f $(DESTDIR)$(mandir)/$$lang/man1/$${file}DLG.1 ; \
			echo " $(LN_S) $(mandir)/$$lang/man1/$${file}.1 $(DESTDIR)$(mandir)/$$lang/man1/$${file}DLG.1" ; \
			$(LN_S) $(mandir)/$$lang/man1/$${file}.1 $(DESTDIR)$(mandir)/$$lang/man1/$${file}DLG.1 ; \
		done ; \
	done

uninstall-hook:
	@list='$(dlg_MANS)'; \
	for file in $$list; do \
		echo " rm -f $(DESTDIR)$(man1dir)/$${file}DLG.1" ; \
		rm -f $(DESTDIR)$(man1dir)/$${file}DLG.1 ; \
	done
	@for lang in $(man_langs); do \
		list='$(dist_man_MANS)' ; \
		for file in $$list; do \
			name=`echo $$file | sed -e "s/\.1/\.$$lang\.1/"` ; \
			echo " rm -f $(DESTDIR)$(mandir)/$$lang/man1/$$file" ; \
			rm -f $(DESTDIR)$(mandir)/$$lang/man1/$$file ; \
		done ; \
		list='$(dlg_MANS)'; \
		for file in $$list; do \
			echo " rm -f $(DESTDIR)$(mandir)/$$lang/man1/$${file}DLG.1" ; \
			rm -f $(DESTDIR)$(mandir)/$$lang/man1/$${file}DLG.1 ; \
		done ; \
	done
