DEFS = -DHAVE_CONFIG_H

# Sources

noinst_LIBRARIES = \
	libmulecommon.a

# Common to external apps and core/gui/monolithic

libmulecommon_a_SOURCES = \
	ConvAmule.cpp \
	Format.cpp \
	MD5Sum.cpp \
	MuleDebug.cpp \
	PlatformSpecific.cpp \
	StringFunctions.cpp
	
libmulecommon_a_CFLAGS = $(WX_CXXFLAGS) $(BFD_FLAGS) 
libmulecommon_a_CXXFLAGS = $(WX_CXXFLAGS) $(BFD_FLAGS) 

noinst_HEADERS =  \
		ConvAmule.h \
		MD5Sum.h \
		MuleDebug.h \
		PlatformSpecific.h \
		StringFunctions.h

MAINTAINERCLEANFILES = Makefile.in

