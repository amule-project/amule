bin_PROGRAMS = amuleweb

amuleweb_SOURCES =  \
	WebServer.cpp \
	WebInterface.cpp \
	WebSocket.cpp \
	$(top_srcdir)/src/ExternalConnector.cpp \
	$(top_srcdir)/src/OtherFunctions.cpp \
	$(top_srcdir)/src/RLE.cpp \
	$(top_srcdir)/src/NetworkFunctions.cpp \
	$(top_srcdir)/src/UPnP.cpp \
	UPnPCompatibility.cpp \
	php_parser.c \
	php_lexer.c \
	php_syntree.cpp \
	php_core_lib.cpp

php_parser.c: php_parser.y
	bison --debug -t -d -v -o $@ $<

php_lexer.c: php_lexer.l
	lex -o$@ $<

amuleweb_DEPENDENCIES = ../../libs/common/libmulecommon.a ../../libs/ec/cpp/libec.a
amuleweb_CFLAGS = -I../.. -I$(top_srcdir)/src -I$(top_srcdir)/src/libs \
	$(WXBASE_CFLAGS) $(LIBPNG_CFLAGS) $(AMULEWEB_DEFS) $(BFD_FLAGS) \
	-DEC_REMOTE -DWEBSERVERDIR="\"$(pkgdatadir)/webserver\"" \
	-DAMULE_INSTALL_PREFIX="\"$(prefix)\"" \
	-DAMULE_LOCALEDIR="\"$(datadir)/locale\"" \
	-DECSOCKET_USE_EVENTS=0
amuleweb_CXXFLAGS = -I../.. -I$(top_srcdir)/src -I$(top_srcdir)/src/libs \
	$(WXBASE_CXXFLAGS) $(LIBPNG_CXXFLAGS) $(AMULEWEB_DEFS) $(BFD_FLAGS) \
	-DEC_REMOTE -DWEBSERVERDIR="\"$(pkgdatadir)/webserver\"" \
	-DAMULE_INSTALL_PREFIX="\"$(prefix)\"" \
	-DAMULE_LOCALEDIR="\"$(datadir)/locale\"" \
	-DECSOCKET_USE_EVENTS=0
amuleweb_LDFLAGS = $(LIBPNG_LDFLAGS) -L../../libs/common -L../../libs/ec/cpp
amuleweb_LDADD = -lec -lmulecommon $(WXBASE_LIBS) $(ZLIB_LIBS) \
	$(READLINE_LIBS) $(LIBPNG_LIBS) $(RESOLV_LIB) $(BFD_LIB)

noinst_HEADERS = \
		 WebInterface.h \
		 WebServer.h \
		 WebSocket.h
