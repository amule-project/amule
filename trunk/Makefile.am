SUBDIRS = m4 docs src unittests

if COMPILE_NLS
SUBDIRS += intl po
endif

EXTRA_DIST = ABOUT-NLS config.rpath mkinstalldirs \
	config.h.in Compilation.flags.in acinclude.m4 \
	amule.rc MakefileWIN32 configWIN32.h amule.msp \
	amule.ico

dist_icon_DATA = 
icondir = $(datadir)/pixmaps

dist_util_DATA = 
utildir = $(datadir)/applications

if MONOLITHIC
dist_icon_DATA += amule.xpm
dist_util_DATA += amule.desktop
endif

if AMULE_GUI
dist_icon_DATA += amulegui.xpm
dist_util_DATA += amulegui.desktop
endif

ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS = --enable-amulecmd --enable-webserver

# Avoid everything getting built when doing "make check"
check-recursive: SUBDIRS = unittests

MAINTAINERCLEANFILES = ABOUT-NLS \
					   Makefile.in \
					   aclocal.m4 \
					   config.guess \
					   config.h.in \
					   config.rpath \
					   config.sub \
					   configure \
					   depcomp \
					   install-sh \
					   missing \
					   mkinstalldirs
