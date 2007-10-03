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

# Resources

if NEED_RC

SUFFIXES = .rc

.rc.$(OBJEXT):
	$(RC) $(RCFLAGS) -I$(top_srcdir) -D__WIN95__ -D__WIN32__ -D__GNUWIN32__ -O COFF -i "$<" -o "$@"

nodist_cas_SOURCES = cas-version.rc

cas-version.rc: $(top_builddir)/version.rc
	@sed -e 's/VER_FILEDESCRIPTION_STR/"C aMule Statistics"/' \
	     -e 's/VER_INTERNALNAME_STR/"cas"/' \
	     -e 's/VER_ORIGINALFILENAME_STR/"cas$(EXEEXT)"/' "$<" > "$@"

CLEANFILES = *-version.rc

endif

MAINTAINERCLEANFILES = Makefile.in
