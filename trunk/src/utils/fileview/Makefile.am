noinst_PROGRAMS = mulefileview

mulefileview_SOURCES = \
	../../CFile.cpp \
	../../SafeFile.cpp \
	../../Tag.cpp \
	../../kademlia/utils/UInt128.cpp \
	FileView.cpp \
	KadFiles.h \
	KadFiles.cpp \
	Print.h \
	Print.cpp

mulefileview_DEPENDENCIES = ../../libs/common/libmulecommon.a
mulefileview_CPPFLAGS = -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/include
mulefileview_CXXFLAGS = $(WXBASE_CXXFLAGS)
mulefileview_LDFLAGS = -L../../libs/common
mulefileview_LDADD = -lmulecommon $(WXBASE_LIBS) $(CRYPTOLIBS)
