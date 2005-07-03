# If any tests uses more than a cpp/h fileset, then 
# make a seperate subfolder for it to reduce clutter.
#SUBDIRS = 


# Use this to be able to catch assertions as exceptions
MUCXXFLAGS = -DMULEUNIT
AM_CXXFLAGS = -I$(srcdir) -I$(srcdir)/.. -I$(top_srcdir)/src $(WX_CXXFLAGS) ${MUCXXFLAGS}
LDADD = $(WX_LIBS) ../muleunit/libmuleunit.a

MAINTAINERCLEANFILES = Makefile.in
TESTS = RangeMapTest 
check_PROGRAMS = $(TESTS)


# Tests for the CRangeMap class
RangeMapTest_SOURCES = RangeMapTest.cpp
RangeMapTest_CXXFLAGS = $(AM_CXXFLAGS)
RangeMapTest_LDADD = $(LDADD)

