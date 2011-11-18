AM_CPPFLAGS = $(MULECPPFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/include $(WXBASE_CPPFLAGS) $(ZLIB_CPPFLAGS)
AM_CXXFLAGS = $(MULECXXFLAGS) $(WX_CFLAGS_ONLY) $(WX_CXXFLAGS_ONLY)

EXTRA_DIST = gen_ECVersion

# Sources

noinst_LIBRARIES = \
	libec.a

libec_a_SOURCES = \
	ECTag.cpp \
	ECPacket.cpp \
	ECSocket.cpp \
	ECMuleSocket.cpp \
	RemoteConnect.cpp \
	ECSpecialTags.cpp

noinst_HEADERS =  \
		ECTag.h \
		ECSocket.h \
		ECMuleSocket.h \
		ECPacket.h \
		ECFileConfig.h \
		ECSpecialTags.h \
		ECCodes.h \
		ECTagTypes.h \
		ECID.h \
		RemoteConnect.h

MAINTAINERCLEANFILES = Makefile.in

BUILT_SOURCES = ECVersion.h

CLEANFILES = ECVersion.h EC-timestamp mkFileSum$(BUILD_EXEEXT)

mkFileSum$(BUILD_EXEEXT): $(top_srcdir)/src/utils/mkFileSum.c
	$(BUILD_CC) $(BUILD_CFLAGS) $(BUILD_CPPFLAGS) $(BUILD_LDFLAGS) $^ -o $@

ECVersion.h: EC-timestamp
	@if test ! -f $@; then \
	  rm -f EC-timestamp; \
	  $(MAKE) EC-timestamp; \
	else :; fi

EC-timestamp: mkFileSum$(BUILD_EXEEXT) $(top_srcdir)/src/libs/ec/cpp/gen_ECVersion $(top_srcdir)/src/libs/ec/cpp/ECCodes.h
	@echo "ECVersion.h timestamp file" > EC-timestamp
	@$(top_srcdir)/src/libs/ec/cpp/gen_ECVersion $(top_srcdir)
