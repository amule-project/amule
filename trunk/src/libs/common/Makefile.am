DEFS = -DHAVE_CONFIG_H

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
	Path.cpp
	
libmulecommon_a_CFLAGS = $(WX_CXXFLAGS) $(BFD_FLAGS) 
libmulecommon_a_CXXFLAGS = $(WX_CXXFLAGS) $(BFD_FLAGS) 

noinst_HEADERS =  \
		Format.h \
		MD5Sum.h \
		MuleDebug.h \
		StringFunctions.h \
		FileFunctions.h \
		Path.h

MAINTAINERCLEANFILES = Makefile.in

