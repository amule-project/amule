DEFS = -DHAVE_CONFIG_H

# Sources

noinst_LIBRARIES = \
	libec.a

libec_a_SOURCES = \
	ECPacket.cpp \
	ECSocket.cpp \
	RemoteConnect.cpp \
	ECSpecialTags.cpp

libec_a_CFLAGS = $(WX_CXXFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src/libs
libec_a_CXXFLAGS = $(WX_CXXFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src/libs

libec_noevt_a_CFLAGS = $(WX_CXXFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src/libs -DECSOCKET_USE_EVENTS=0

noinst_HEADERS =  \
		ECSocket.h \
		ECPacket.h \
		ECFileConfig.h \
		ECSpecialTags.h \
		ECCodes.h \
		RemoteConnect.h

MAINTAINERCLEANFILES = Makefile.in

BUILT_SOURCES = ECVersion.h

CLEANFILES = ECVersion.h EC-timestamp mkFileSum$(EXEEXT)

mkFileSum$(EXEEXT): $(top_srcdir)/src/utils/mkFileSum.c
	$(CC) -o $@ $<

ECVersion.h: EC-timestamp
	@if test ! -f $@; then \
	  rm -f EC-timestamp; \
	  $(MAKE) EC-timestamp; \
	else :; fi

EC-timestamp: mkFileSum$(EXEEXT) $(top_srcdir)/src/libs/ec/gen_ECVersion $(top_srcdir)/src/libs/ec/ECCodes.h
	@echo "ECVersion.h timestamp file" > EC-timestamp
	@$(top_srcdir)/src/libs/ec/gen_ECVersion $(top_srcdir)
