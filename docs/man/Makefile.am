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

install-data-hook:
	list='$(dlg_MANS)'; \
	for file in $$list; do \
		rm -f $(DESTDIR)$(man1dir)/$${file}DLG.1 ; \
		$(LN_S) $(man1dir)/$${file}.1 $(DESTDIR)$(man1dir)/$${file}DLG.1 ;\
	done

uninstall-hook:
	list='$(dlg_MANS)'; \
	for file in $$list; do \
		rm -f $(DESTDIR)$(man1dir)/$${file}DLG.1 ; \
	done
