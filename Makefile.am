SUBDIRS = . m4 intl po docs src

EXTRA_DIST = ABOUT-NLS config.rpath mkinstalldirs \
	config.h.in Compilation.flags.in acinclude.m4

dist_icon_DATA = amule.xpm
icondir = $(datadir)/pixmaps

dist_util_DATA = amule.desktop
utildir = $(datadir)/applications

ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS = --enable-amulecmd --enable-amulecmdgui --enable-webserver --enable-webservergui
