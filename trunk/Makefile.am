SUBDIRS = m4 docs src unittests

if COMPILE_NLS
SUBDIRS += intl po
endif

EXTRA_DIST = ABOUT-NLS config.rpath mkinstalldirs \
	config.h.in Compilation.flags.in acinclude.m4 \
	amule.rc MakefileWIN32 configWIN32.h amule.msp \
	amule.ico

dist_icon_DATA = amule.xpm
icondir = $(datadir)/pixmaps

dist_util_DATA = amule.desktop
utildir = $(datadir)/applications

ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS = --enable-amulecmd --enable-amulecmdgui --enable-webserver --enable-webservergui


# Avoid everything getting built when doing "make check"
check-recursive: SUBDIRS = unittests
