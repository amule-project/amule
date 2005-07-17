bin_PROGRAMS = 

if WEB
bin_PROGRAMS += amuleweb
endif

if WEBGUI
bin_PROGRAMS += amulewebDLG
endif


amuleweb_SOURCES =  \
	WebServer.cpp \
	WebInterface.cpp \
	WebSocket.cpp \
	$(top_srcdir)/src/MD5Sum.cpp \
	$(top_srcdir)/src/ECSocket.cpp \
	$(top_srcdir)/src/ExternalConnector.cpp \
	$(top_srcdir)/src/ECPacket.cpp \
	$(top_srcdir)/src/OtherFunctions.cpp \
	$(top_srcdir)/src/ECSpecialTags.cpp \
	$(top_srcdir)/src/RLE.cpp \
	$(top_srcdir)/src/StringFunctions.cpp \
	$(top_srcdir)/src/NetworkFunctions.cpp \
	$(top_srcdir)/src/Format.cpp

amuleweb_CFLAGS = -I$(top_srcdir)/src/ $(WXBASE_CFLAGS) $(LIBPNG_CFLAGS) $(AMULEWEB_DEFS) $(BFD_FLAGS) -DEC_REMOTE -DWEBSERVERDIR="\"$(pkgdatadir)/webserver\""
amuleweb_CXXFLAGS = -I$(top_srcdir)/src/ $(WXBASE_CXXFLAGS) $(LIBPNG_CXXFLAGS) $(AMULEWEB_DEFS) $(BFD_FLAGS) -DEC_REMOTE -DWEBSERVERDIR="\"$(pkgdatadir)/webserver\""
amuleweb_LDADD = $(WXBASE_LIBS) $(ZLIB_LIBS) $(READLINE_LIBS) $(LIBPNG_LDFLAGS) $(LIBPNG_LIBS) $(RESOLV_LIB) $(BFD_LIB)

amulewebDLG_SOURCES =  \
	WebServer.cpp \
	WebInterface.cpp \
	WebSocket.cpp \
	$(top_srcdir)/src/MD5Sum.cpp \
	$(top_srcdir)/src/ECSocket.cpp \
	$(top_srcdir)/src/ExternalConnector.cpp \
	$(top_srcdir)/src/ECPacket.cpp \
	$(top_srcdir)/src/OtherFunctions.cpp \
	$(top_srcdir)/src/ECSpecialTags.cpp \
	$(top_srcdir)/src/RLE.cpp \
	$(top_srcdir)/src/StringFunctions.cpp \
	$(top_srcdir)/src/NetworkFunctions.cpp \
	$(top_srcdir)/src/Format.cpp

amulewebDLG_CFLAGS = -I$(top_srcdir)/src/ $(WX_CFLAGS) $(LIBPNG_CFLAGS) $(AMULEWEB_DEFS) $(BFD_FLAGS) -DAMULEWEBDLG -DEC_REMOTE -DWEBSERVERDIR="\"$(pkgdatadir)/webserver\""
amulewebDLG_CXXFLAGS = -I$(top_srcdir)/src/ $(WX_CXXFLAGS) $(LIBPNG_CXXFLAGS) $(AMULEWEB_DEFS) $(BFD_FLAGS) -DAMULEWEBDLG  -DEC_REMOTE -DWEBSERVERDIR="\"$(pkgdatadir)/webserver\""
amulewebDLG_LDADD = $(WX_LIBS) $(ZLIB_LIBS) $(LIBPNG_LDFLAGS) $(LIBPNG_LIBS) $(RESOLV_LIB) $(BFD_LIB)
