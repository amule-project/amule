bin_PROGRAMS = wxcas

wxcas_SOURCES = onlinesig.cpp \
		linuxmon.cpp \
		wxcas.cpp \
		wxcasprint.cpp \
		wxcasframe.cpp \
		wxcasprefs.cpp \
		wxcascte.cpp \
		wxcaspix.cpp	

noinst_HEADERS = onlinesig.h \
		linuxmon.h \
		wxcas.h \
		wxcasprint.h \
		wxcasframe.h \
		wxcasprefs.h \
		wxcascte.h \
		wxcaspix.h

wxcas_CFLAGS = $(WX_CFLAGS)
wxcas_CXXFLAGS = $(WX_CXXFLAGS)
wxcas_LDADD = $(WX_LIBS)

# Resources

if NEED_RC

SUFFIXES = .rc

wxcas_LDADD += wxcasrc.$(OBJEXT)

wxcasrc.$(OBJEXT): $(srcdir)/../wxcas.rc wxcas-version.rc
	cat $^ | \
	( \
		abs_builddir=`pwd` ; \
		abs_top_srcdir=`cd $(top_srcdir) ; pwd` ; \
		cd $(<D) ; \
		$(RC) $(RCFLAGS) -I$${abs_top_srcdir} -D__WIN95__ -D__WIN32__ -D__GNUWIN32__ -O COFF -o "$${abs_builddir}/$@" ; \
	)

wxcas-version.rc: $(top_builddir)/version.rc
	@sed -e 's/VER_FILEDESCRIPTION_STR/"Graphical aMule Statistics"/' \
	     -e 's/VER_INTERNALNAME_STR/"wxcas"/' \
	     -e 's/VER_ORIGINALFILENAME_STR/"wxcas$(EXEEXT)"/' "$<" > "$@"

CLEANFILES = *-version.rc

endif

MAINTAINERCLEANFILES = Makefile.in
