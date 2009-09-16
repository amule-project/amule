# If any tests uses more than a cpp/h fileset, then 
# make a seperate subfolder for it to reduce clutter.
#SUBDIRS = 
EXTRA_DIST =

# Use this to be able to catch assertions as exceptions
MUCPPFLAGS = -DMULEUNIT
AM_CPPFLAGS = $(MULECPPFLAGS) -I$(srcdir) -I$(srcdir)/.. -I$(top_srcdir)/src -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/include $(MUCPPFLAGS) $(WXBASE_CPPFLAGS)
AM_CXXFLAGS = $(MULECXXFLAGS) $(WX_CFLAGS_ONLY) $(WX_CXXFLAGS_ONLY)
AM_LDFLAGS = $(MULELDFLAGS)
LDADD = $(WXBASE_LIBS) ../muleunit/libmuleunit.a

MAINTAINERCLEANFILES = Makefile.in
TESTS = CUInt128Test RangeMapTest FormatTest StringFunctionsTest NetworkFunctionsTest FileDataIOTest PathTest TextFileTest
check_PROGRAMS = $(TESTS)


# Tests for the CUInt128 class
CUInt128Test_SOURCES = CUInt128Test.cpp $(top_srcdir)/src/kademlia/utils/UInt128.cpp
CUInt128Test_CPPFLAGS = $(AM_CPPFLAGS) $(CRYPTOPP_CPPFLAGS)
CUInt128Test_LDFLAGS = $(AM_LDFLAGS) $(CRYPTOPP_LDFLAGS)
CUInt128Test_LDADD = $(LDADD) $(CRYPTOPP_LIBS)

# Tests for the CRangeMap class
RangeMapTest_SOURCES = RangeMapTest.cpp

# Tests for the CFormat class
FormatTest_SOURCES = FormatTest.cpp $(top_srcdir)/src/libs/common/Format.cpp

# Tests for the various string functions.
StringFunctionsTest_SOURCES = StringFunctionsTest.cpp $(top_srcdir)/src/libs/common/StringFunctions.cpp $(top_srcdir)/src/libs/common/Path.cpp

# Tests for the various file functions.
#FileFunctionsTest_SOURCES = FileFunctionsTest.cpp $(top_srcdir)/src/FileFunctions.cpp $(top_srcdir)/src/SafeFile.cpp $(top_srcdir)/src/CFile.cpp $(top_srcdir)/src/kademlia/utils/UInt128.cpp $(top_srcdir)/src/CryptoPP.cpp

# Tests for the various network functions.
NetworkFunctionsTest_SOURCES = NetworkFunctionsTest.cpp $(top_srcdir)/src/NetworkFunctions.cpp
NetworkFunctionsTest_CPPFLAGS = $(AM_CPPFLAGS) -DEC_REMOTE # Needed to avoid compiling the http-thread

# Tests for the classes that implement the CFileDataIO interface
FileDataIOTest_SOURCES = FileDataIOTest.cpp $(top_srcdir)/src/SafeFile.cpp $(top_srcdir)/src/CFile.cpp $(top_srcdir)/src/MemFile.cpp $(top_srcdir)/src/kademlia/utils/UInt128.cpp $(top_srcdir)/src/libs/common/StringFunctions.cpp $(top_srcdir)/src/Tag.cpp $(top_srcdir)/src/libs/common/Path.cpp
FileDataIOTest_CPPFLAGS = $(AM_CPPFLAGS) $(CRYPTOPP_CPPFLAGS)
FileDataIOTest_LDFLAGS = $(AM_LDFLAGS) $(CRYPTOPP_LDFLAGS)
FileDataIOTest_LDADD = $(LDADD) $(CRYPTOPP_LIBS)

# Tests for the CPath class
PathTest_SOURCES = PathTest.cpp $(top_srcdir)/src/libs/common/Path.cpp $(top_srcdir)/src/libs/common/StringFunctions.cpp
PathTest_CPPFLAGS = $(AM_CPPFLAGS) $(CRYPTOPP_CPPFLAGS)
PathTest_LDFLAGS = $(AM_LDFLAGS) $(CRYPTOPP_LDFLAGS)
PathTest_LDADD = $(LDADD) $(CRYPTOPP_LIBS)

# Tests for the CTextFile class
TextFileTest_SOURCES = TextFileTest.cpp $(top_srcdir)/src/libs/common/Path.cpp $(top_srcdir)/src/libs/common/StringFunctions.cpp $(top_srcdir)/src/libs/common/TextFile.cpp
TextFileTest_CPPFLAGS = $(AM_CPPFLAGS) $(CRYPTOPP_CPPFLAGS) -DSRCDIR="$(srcdir)"
TextFileTest_LDFLAGS = $(AM_LDFLAGS) $(CRYPTOPP_LDFLAGS)
TextFileTest_LDADD = $(LDADD) $(CRYPTOPP_LIBS)
EXTRA_DIST += TextFileTest_dos.txt TextFileTest_unix.txt
