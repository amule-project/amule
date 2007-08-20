EXTRA_DIST = README

SUBDIRS = docs

bin_PROGRAMS = cas

dist_cas_DATA = tmp.html stat.png
casdir = $(datadir)/cas

cas_SOURCES = cas.c \
	      configfile.c \
	      functions.c \
	      graphics.c \
	      html.c \
	      lines.c

noinst_HEADERS = configfile.h \
	      functions.h \
	      graphics.h \
	      html.h \
	      lines.h \
	      version.h

cas_CFLAGS= $(GDLIB_CFLAGS) $(CAS_DEFS)
cas_LDADD= $(GDLIB_LDFLAGS) $(GDLIB_LIBS) 

if NEED_CORESERVICES
cas_LDFLAGS = -framework CoreServices
endif

MAINTAINERCLEANFILES = Makefile.in
