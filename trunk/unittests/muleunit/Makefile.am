noinst_LIBRARIES = libmuleunit.a
libmuleunit_a_SOURCES = \
	test.cpp \
	test.h \
	main.cpp \
	testcase.cpp \
	testcase.h \
	testregistry.cpp \
	testregistry.h \
	$(top_srcdir)/src/libs/common/MuleDebug.cpp

libmuleunit_a_CXXFLAGS = $(WXBASE_CXXFLAGS) -I$(top_srcdir)/src/libs -DMULEUNIT 

EXTRA_DIST = license.txt

MAINTAINERCLEANFILES = Makefile.in
