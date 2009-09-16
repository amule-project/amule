AM_CPPFLAGS = $(MULECPPFLAGS) $(BFD_CPPFLAGS) $(WX_CPPFLAGS)
AM_CXXFLAGS = $(MULECXXFLAGS) $(WX_CFLAGS_ONLY) $(WX_CXXFLAGS_ONLY)

# Sources

noinst_LIBRARIES = \
	libmulecommon.a

# Common to external apps and core/gui/monolithic

libmulecommon_a_SOURCES = \
	Format.cpp \
	MD5Sum.cpp \
	MuleDebug.cpp \
	StringFunctions.cpp \
	FileFunctions.cpp \
	TextFile.cpp \
	Path.cpp

noinst_HEADERS =  \
		Format.h \
		MD5Sum.h \
		MuleDebug.h \
		StringFunctions.h \
		FileFunctions.h \
		TextFile.h \
		Path.h

MAINTAINERCLEANFILES = Makefile.in

