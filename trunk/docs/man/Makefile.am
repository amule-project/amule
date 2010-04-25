SELECTED_MANPAGES = \
	$(AMULECMD_MANPAGES) \
	$(WEBSERVER_MANPAGES) \
	$(AMULE_GUI_MANPAGES) \
	$(MONOLITHIC_MANPAGES) \
	$(AMULE_DAEMON_MANPAGES) \
	$(ED2K_MANPAGES)

EXTRA_DIST = \
	amule.1 amuled.1 amulecmd.1 amulegui.1 amuleweb.1 ed2k.1 \
	amule.de.1 amuled.de.1 amulecmd.de.1 amulegui.de.1 amuleweb.de.1 ed2k.de.1 \
	amule.es.1 amuled.es.1 amulecmd.es.1 amulegui.es.1 amuleweb.es.1 ed2k.es.1 \
	amule.eu.1 amuled.eu.1 amulecmd.eu.1 amuleweb.eu.1 ed2k.eu.1 \
	amule.fr.1 amuled.fr.1 amulecmd.fr.1 amuleweb.fr.1 ed2k.fr.1 \
	amule.hu.1 amuled.hu.1 amulecmd.hu.1 amulegui.hu.1 amuleweb.hu.1 ed2k.hu.1 \
	amule.it.1 amuled.it.1 amulecmd.it.1 amulegui.it.1 amuleweb.it.1 ed2k.it.1

EXTRA_DIST += po4a.config

EXTRA__DIST__SUBDIRS = po

include $(top_srcdir)/automake/dist-hook.am
include $(top_srcdir)/automake/manpages.am

MAINTAINERCLEANFILES = Makefile.in
