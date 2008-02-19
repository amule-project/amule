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

libec_a_CFLAGS = $(WX_CXXFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/include
libec_a_CXXFLAGS = $(WX_CXXFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/include

libec_noevt_a_CFLAGS = $(WX_CXXFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/include -DECSOCKET_USE_EVENTS=0

noinst_HEADERS =  \
		ECTag.h \
		ECSocket.h \
		ECMuleSocket.h \
		ECPacket.h \
		ECFileConfig.h \
		ECSpecialTags.h \
		ECCodes.h \
		ECTagTypes.h \
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
