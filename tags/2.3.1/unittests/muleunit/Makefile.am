noinst_LIBRARIES = libmuleunit.a
libmuleunit_a_SOURCES = \
	test.cpp \
	test.h \
	main.cpp \
	testcase.cpp \
	testcase.h \
	testregistry.cpp \
	testregistry.h \
	$(top_srcdir)/src/libs/common/MuleDebug.cpp \
	$(top_srcdir)/src/libs/common/StringFunctions.cpp

libmuleunit_a_CPPFLAGS = $(MULECPPFLAGS) $(WXBASE_CPPFLAGS) -I$(top_srcdir)/src/libs -DMULEUNIT 
libmuleunit_a_CXXFLAGS = $(MULECXXFLAGS) $(WX_CFLAGS_ONLY) $(WX_CXXFLAGS_ONLY)

EXTRA_DIST = license.txt

MAINTAINERCLEANFILES = Makefile.in
