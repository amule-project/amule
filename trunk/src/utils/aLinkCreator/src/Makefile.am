bin_PROGRAMS =

if ALC
bin_PROGRAMS += alc
endif

if ALCC
bin_PROGRAMS += alcc
endif

alc_SOURCES = alc.cpp \
		alcframe.cpp \
		ed2khash.cpp \
		alcpix.cpp \
		md4.cpp

noinst_HEADERS = alc.h \
		alcc.h \
		alcframe.h \
		ed2khash.h \
		alcpix.h \
		md4.h \
		bithelp.h

alc_CFLAGS = $(WX_CFLAGS)
alc_CXXFLAGS = $(WX_CXXFLAGS)
alc_LDADD = $(WX_LIBS)


alcc_SOURCES = alcc.cpp \
		ed2khash.cpp \
		md4.cpp

alcc_CFLAGS = $(WXBASE_CFLAGS)
alcc_CXXFLAGS = $(WXBASE_CXXFLAGS)
alcc_LDADD = $(WXBASE_LIBS)

# Resources

if NEED_RC

SUFFIXES = .rc

.rc.$(OBJEXT):
	$(RC) $(RCFLAGS) -I$(top_srcdir) -D__WIN95__ -D__WIN32__ -D__GNUWIN32__ -O COFF -i "$<" -o "$@"

nodist_alcc_SOURCES = alcc-version.rc
alc_LDADD += alcrc.$(OBJEXT)

alcrc.$(OBJEXT): $(srcdir)/../alc.rc alc-version.rc
	cat $^ | \
	( \
		abs_builddir=`pwd` ; \
		abs_top_srcdir=`cd $(top_srcdir) ; pwd` ; \
		cd $(<D) ; \
		$(RC) $(RCFLAGS) -I$${abs_top_srcdir} -D__WIN95__ -D__WIN32__ -D__GNUWIN32__ -O COFF -o "$${abs_builddir}/$@" ; \
	)

alc-version.rc: $(top_builddir)/version.rc
	@sed -e 's/VER_FILEDESCRIPTION_STR/"aMule Link Creator"/' \
	     -e 's/VER_INTERNALNAME_STR/"alc"/' \
	     -e 's/VER_ORIGINALFILENAME_STR/"alc$(EXEEXT)"/' "$<" > "$@"

alcc-version.rc: $(top_builddir)/version.rc
	@sed -e 's/VER_FILEDESCRIPTION_STR/"aMule Link Creator, Console"/' \
	     -e 's/VER_INTERNALNAME_STR/"alcc"/' \
	     -e 's/VER_ORIGINALFILENAME_STR/"alcc$(EXEEXT)"/' "$<" > "$@"

CLEANFILES = *-version.rc

endif

MAINTAINERCLEANFILES = Makefile.in
