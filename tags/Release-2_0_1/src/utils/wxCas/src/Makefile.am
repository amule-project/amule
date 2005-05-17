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

if NEED_RC
wxcas_LDADD += wxcasrc.$(OBJEXT)

wxcasrc.$(OBJEXT): $(srcdir)/../wxcas.rc
	@thisdir=`pwd` ; \
	cd $(srcdir)/.. ; \
	rcfile=`basename "$<"` ; \
	echo "windres $(RCFLAGS) --define __WIN95__ --define __WIN32__ --define __GNUWIN32__ -O COFF -i "$${rcfile}" -o \"$@\"" ; \
	windres $(RCFLAGS) --define __WIN95__ --define __WIN32__ --define __GNUWIN32__ -O COFF -i "$${rcfile}" -o "$@" ; \
	cd $${thisdir} ; \
	mv "$(srcdir)/../$@" "$@"
endif
