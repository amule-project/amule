noinst_LIBRARIES = libmuleunit.a
libmuleunit_a_SOURCES = \
	test.cpp \
	testresult.cpp \
	main.cpp \
	testcase.cpp \
	testregistry.cpp \
	$(top_srcdir)/src/libs/common/MuleDebug.cpp

libmuleunit_a_CXXFLAGS = $(WX_CXXFLAGS) -I$(top_srcdir)/src/libs/common/ -DMULEUNIT 
