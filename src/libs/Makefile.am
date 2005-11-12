# SUBDIRS = ec common

# bin_PROGRAMS = 

DEFS = -DHAVE_CONFIG_H

# Sources

noinst_LIBRARIES = \
	libmulecommon.a \
	libec.a

# Common to external apps and core/gui/monolithic

libmulecommon_a_SOURCES = \
	common/Format.cpp \
	common/MD5Sum.cpp \
	common/MuleDebug.cpp \
	common/PlatformSpecific.cpp \
	common/StringFunctions.cpp
	
libmulecommon_a_CFLAGS = $(WX_CXXFLAGS) $(BFD_FLAGS) 
libmulecommon_a_CXXFLAGS = $(WX_CXXFLAGS) $(BFD_FLAGS) 

# Common to all EC communication code. Ideally all EC should be here

libec_a_SOURCES = \
	ec/ECPacket.cpp \
	ec/ECSocket.cpp \
	ec/RemoteConnect.cpp
	
libec_a_CFLAGS = $(WX_CXXFLAGS) -I$(top_srcdir)/src -I..
libec_a_CXXFLAGS = $(WX_CXXFLAGS) -I$(top_srcdir)/src -I..

noinst_HEADERS =  \
		ec/ECSocket.h \
		ec/ECPacket.h \
		ec/ECSpecialTags.h \
		ec/ECcodes.h \
		ec/RemoteConnect.h \
		common/MD5Sum.h \
		common/MuleDebug.h \
		common/PlatformSpecific.h \
		common/StringFunctions.h

MAINTAINERCLEANFILES = Makefile.in
