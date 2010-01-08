SUBDIRS =

if COMPILE_NLS
SUBDIRS += intl po
endif

SUBDIRS += docs src unittests

EXTRA_DIST = amule.rc amule.ico amule.png convert.ico \
	README.Debian-Packages aMule-CVS.spec aMule.spec \
	amule_build_install.sh autogen.sh \
	po/l10n.xsl

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

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-amulecmd \
	--enable-webserver \
	--enable-amule-daemon \
	--enable-amule-gui \
	--enable-cas \
	--enable-wxcas \
	--enable-plasmamule \
	--enable-alc \
	--enable-alcc

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

EXTRA__DIST__SUBDIRS = aMule.app debian
include $(top_srcdir)/automake/dist-hook.am
