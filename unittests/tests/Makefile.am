# If any tests uses more than a cpp/h fileset, then 
# make a seperate subfolder for it to reduce clutter.
#SUBDIRS = 


# Use this to be able to catch assertions as exceptions
MUCXXFLAGS = -DMULEUNIT 
AM_CXXFLAGS = -I$(srcdir) -I$(srcdir)/.. -I$(top_srcdir)/src $(WX_CXXFLAGS) ${MUCXXFLAGS} 
LDADD = $(WX_LIBS) ../muleunit/libmuleunit.a

MAINTAINERCLEANFILES = Makefile.in
TESTS = RangeMapTest FormatTest StringFunctionsTest NetworkFunctionsTest FileDataIOTest
check_PROGRAMS = $(TESTS)


# Tests for the CRangeMap class
RangeMapTest_SOURCES = RangeMapTest.cpp
RangeMapTest_CXXFLAGS = $(AM_CXXFLAGS)
RangeMapTest_LDADD = $(LDADD)


# Tests for the CFormat class
FormatTest_SOURCES = FormatTest.cpp $(top_srcdir)/src/Format.cpp
FormatTest_CXXFLAGS = $(AM_CXXFLAGS)
FormatTest_LDADD = $(LDADD)


# Tests for the various string functions.
StringFunctionsTest_SOURCES = StringFunctionsTest.cpp $(top_srcdir)/src/StringFunctions.cpp
StringFunctionsTest_CXXFLAGS = $(AM_CXXFLAGS)
StringFunctionsTest_LDADD = $(LDADD)

# Tests for the various network functions.
NetworkFunctionsTest_SOURCES = NetworkFunctionsTest.cpp $(top_srcdir)/src/NetworkFunctions.cpp
NetworkFunctionsTest_CXXFLAGS = $(AM_CXXFLAGS) -DEC_REMOTE # Needed to avoid compiling the http-thread
NetworkFunctionsTest_LDADD = $(LDADD)

# Tests for the classes that implement the CFileDataIO interface
FileDataIOTest_SOURCES = FileDataIOTest.cpp $(top_srcdir)/src/SafeFile.cpp $(top_srcdir)/src/CFile.cpp $(top_srcdir)/src/MemFile.cpp $(top_srcdir)/src/kademlia/utils/UInt128.cpp $(top_srcdir)/src/CryptoPP.cpp $(top_srcdir)/src/FileFunctions.cpp
FileDataIOTest_CXXFLAGS = $(AM_CXXFLAGS) 
FileDataIOTest_LDADD = $(LDADD)
