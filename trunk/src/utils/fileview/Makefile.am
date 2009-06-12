noinst_PROGRAMS = mulefileview

mulefileview_SOURCES = \
	../../CFile.cpp \
	../../SafeFile.cpp \
	../../OtherFunctions.cpp \
	../../Tag.cpp \
	../../kademlia/utils/UInt128.cpp \
	eD2kFiles.h \
	eD2kFiles.cpp \
	FileView.cpp \
	KadFiles.h \
	KadFiles.cpp \
	Print.h \
	Print.cpp

mulefileview_DEPENDENCIES = ../../libs/common/libmulecommon.a
mulefileview_CPPFLAGS = $(MULECPPFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/include $(WXBASE_CPPFLAGS)
mulefileview_CXXFLAGS = $(MULECXXFLAGS) $(WX_CFLAGS_ONLY) $(WX_CXXFLAGS_ONLY)
mulefileview_LDFLAGS = $(MULELDFLAGS) -L../../libs/common
mulefileview_LDADD = -lmulecommon $(WXBASE_LIBS)
