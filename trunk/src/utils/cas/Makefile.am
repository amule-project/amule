EXTRA_DIST = README

bin_PROGRAMS = cas

dist_icon_DATA = stat.png
icondir = $(datadir)/pixmaps

cas_SOURCES = cas.c \
	      functions.c \
	      configfile.c \
	      graphics.c

noinst_HEADERS = configfile.h \
	      functions.h \
	      graphics.h \
	      version.h

cas_CFLAGS= $(GDLIB_CFLAGS) $(CAS_DEFS)
cas_LDADD= $(GDLIB_LDFLAGS) $(GDLIB_LIBS) 
