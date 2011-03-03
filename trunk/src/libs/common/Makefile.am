AM_CPPFLAGS = $(MULECPPFLAGS) $(BFD_CPPFLAGS) $(WXBASE_CPPFLAGS)
AM_CXXFLAGS = $(MULECXXFLAGS) $(WX_CFLAGS_ONLY) $(WX_CXXFLAGS_ONLY)

# Sources

noinst_LIBRARIES = \
	libmulecommon.a

# Common to external apps and core/gui/monolithic

libmulecommon_a_SOURCES = \
	FileFunctions.cpp \
	Format.cpp \
	MD5Sum.cpp \
	MuleDebug.cpp \
	Path.cpp \
	strerror_r.c \
	StringFunctions.cpp \
	TextFile.cpp

noinst_HEADERS =  \
	FileFunctions.h \
	Format.h \
	MD5Sum.h \
	MuleDebug.h \
	Path.h \
	strerror_r.h \
	StringFunctions.h \
	TextFile.h

MAINTAINERCLEANFILES = Makefile.in
