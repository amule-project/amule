noinst_PROGRAMS = mulefileview

mulefileview_SOURCES = \
	../../CFile.cpp \
	../../SafeFile.cpp \
	../../OtherFunctions.cpp \
	../../Tag.cpp \
	../../kademlia/utils/UInt128.cpp \
	../../kademlia/kademlia/SearchManager.h \
	eD2kFiles.h \
	eD2kFiles.cpp \
	FileView.cpp \
	KadFiles.h \
	KadFiles.cpp \
	Print.h \
	Print.cpp

mulefileview_DEPENDENCIES = ../../libs/common/libmulecommon.a
mulefileview_CPPFLAGS = $(MULECPPFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/include $(WXBASE_CPPFLAGS) $(CRYPTOPP_CPPFLAGS)
mulefileview_CXXFLAGS = $(MULECXXFLAGS) $(WX_CFLAGS_ONLY) $(WX_CXXFLAGS_ONLY)
mulefileview_LDFLAGS = $(MULELDFLAGS) -L../../libs/common $(CRYPTOPP_LDFLAGS)
mulefileview_LDADD = -lmulecommon $(WXBASE_LIBS) $(CRYPTOPP_LIBS)
