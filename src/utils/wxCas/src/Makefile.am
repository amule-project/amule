bin_PROGRAMS = wxcas
     
wxcas_SOURCES = onlinesig.cpp \
		linuxmon.cpp \
		wxcas.cpp \
		wxcasprint.cpp \
		wxcasframe.cpp \
		wxcasprefs.cpp \
		wxcascte.cpp
		
if USE_MINGW
  wxcas_SOURCES += wxcas.rc
endif

noinst_HEADERS = onlinesig.h \
		linuxmon.h \
		wxcas.h \
		wxcasprint.h \
		wxcasframe.h \
		wxcasprefs.h \
		wxcascte.h
		
wxcas_CFLAGS = $(WX_CFLAGS)
wxcas_CXXFLAGS = $(WX_CXXFLAGS)
wxcas_LDADD = $(WX_LIBS)
