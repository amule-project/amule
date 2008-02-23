# If any tests uses more than a cpp/h fileset, then 
# make a seperate subfolder for it to reduce clutter.
#SUBDIRS = 
EXTRA_DIST =

# Use this to be able to catch assertions as exceptions
MUCXXFLAGS = -DMULEUNIT -DSRCDIR="$(srcdir)"
AM_CXXFLAGS = -I$(srcdir) -I$(srcdir)/.. -I$(top_srcdir)/src -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/include $(WXBASE_CXXFLAGS) $(MUCXXFLAGS)
LDADD = $(WXBASE_LIBS) ../muleunit/libmuleunit.a

MAINTAINERCLEANFILES = Makefile.in
TESTS = RangeMapTest FormatTest StringFunctionsTest NetworkFunctionsTest FileDataIOTest PathTest TextFileTest
check_PROGRAMS = $(TESTS)


# Tests for the CRangeMap class
RangeMapTest_SOURCES = RangeMapTest.cpp
RangeMapTest_CXXFLAGS = $(AM_CXXFLAGS)
RangeMapTest_LDADD = $(LDADD)


# Tests for the CFormat class
FormatTest_SOURCES = FormatTest.cpp $(top_srcdir)/src/libs/common/Format.cpp
FormatTest_CXXFLAGS = $(AM_CXXFLAGS)
FormatTest_LDADD = $(LDADD)


# Tests for the various string functions.
StringFunctionsTest_SOURCES = StringFunctionsTest.cpp $(top_srcdir)/src/libs/common/StringFunctions.cpp $(top_srcdir)/src/libs/common/Path.cpp
StringFunctionsTest_CXXFLAGS = $(AM_CXXFLAGS)
StringFunctionsTest_LDADD = $(LDADD)

# Tests for the various file functions.
#FileFunctionsTest_SOURCES = FileFunctionsTest.cpp $(top_srcdir)/src/FileFunctions.cpp $(top_srcdir)/src/SafeFile.cpp $(top_srcdir)/src/CFile.cpp $(top_srcdir)/src/kademlia/utils/UInt128.cpp $(top_srcdir)/src/CryptoPP.cpp
#FileFunctionsTest_CXXFLAGS = $(AM_CXXFLAGS)
#FileFunctionsTest_LDADD = $(LDADD)

# Tests for the various network functions.
NetworkFunctionsTest_SOURCES = NetworkFunctionsTest.cpp $(top_srcdir)/src/NetworkFunctions.cpp
NetworkFunctionsTest_CXXFLAGS = $(AM_CXXFLAGS) -DEC_REMOTE # Needed to avoid compiling the http-thread
NetworkFunctionsTest_LDADD = $(LDADD)

# Tests for the classes that implement the CFileDataIO interface
FileDataIOTest_SOURCES = FileDataIOTest.cpp $(top_srcdir)/src/SafeFile.cpp $(top_srcdir)/src/CFile.cpp $(top_srcdir)/src/MemFile.cpp $(top_srcdir)/src/kademlia/utils/UInt128.cpp $(top_srcdir)/src/libs/common/StringFunctions.cpp $(top_srcdir)/src/Tag.cpp $(top_srcdir)/src/libs/common/Path.cpp
FileDataIOTest_CXXFLAGS = $(AM_CXXFLAGS) 
FileDataIOTest_LDADD = $(LDADD) $(CRYPTOLIBS)

# Tests for the CPath class
PathTest_SOURCES = PathTest.cpp $(top_srcdir)/src/libs/common/Path.cpp $(top_srcdir)/src/libs/common/StringFunctions.cpp
PathTest_CXXFLAGS = $(AM_CXXFLAGS) 
PathTest_LDADD = $(LDADD) $(CRYPTOLIBS)

# Tests for the CPath class
TextFileTest_SOURCES = TextFileTest.cpp $(top_srcdir)/src/libs/common/Path.cpp $(top_srcdir)/src/libs/common/StringFunctions.cpp $(top_srcdir)/src/libs/common/TextFile.cpp
TextFileTest_CXXFLAGS = $(AM_CXXFLAGS) 
TextFileTest_LDADD = $(LDADD) $(CRYPTOLIBS)
EXTRA_DIST += TextFileTest_dos.txt TextFileTest_unix.txt
