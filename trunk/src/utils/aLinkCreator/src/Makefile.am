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

if NEED_RC
alc_LDADD += alcrc.$(OBJEXT)

alcrc.$(OBJEXT): $(srcdir)/../alc.rc
	@thisdir=`pwd` ; \
	cd $(srcdir)/.. ; \
	rcfile=`basename "$<"` ; \
	echo "$(RC) $(RCFLAGS) --define __WIN95__ --define __WIN32__ --define __GNUWIN32__ -O COFF -i \"$${rcfile}\" -o \"$@\"" ; \
	$(RC) $(RCFLAGS) --define __WIN95__ --define __WIN32__ --define __GNUWIN32__ -O COFF -i "$${rcfile}" -o "$@" ; \
	cd $${thisdir} ; \
	mv "$(srcdir)/../$@" "$@"
endif

MAINTAINERCLEANFILES = Makefile.in
