bin_PROGRAMS = alc alcc

alc_SOURCES = alc.cpp \
		alcframe.cpp \
		ed2khash.cpp \
		alcpix.cpp \
		md4.cpp

alcc_SOURCES = alcc.cpp \
		ed2khash.cpp \
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

alcc_CFLAGS = $(WXBASE_CFLAGS)
alcc_CXXFLAGS = $(WXBASE_CXXFLAGS)
alcc_LDADD = $(WXBASE_LIBS)
