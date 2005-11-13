bin_PROGRAMS = 

bin_PROGRAMS += amuleweb

amuleweb_SOURCES =  \
	WebServer.cpp \
	WebInterface.cpp \
	WebSocket.cpp \
	$(top_srcdir)/src/ExternalConnector.cpp \
	$(top_srcdir)/src/OtherFunctions.cpp \
	$(top_srcdir)/src/RLE.cpp \
	$(top_srcdir)/src/NetworkFunctions.cpp \
	php_parser.c \
	php_lexer.c \
	php_syntree.cpp \
	php_core_lib.cpp

php_parser.c: php_parser.y
	bison --debug -t -d -v -o php_parser.c php_parser.y

php_lexer.c: php_lexer.l
	lex -ophp_lexer.c php_lexer.l

amuleweb_CFLAGS = -I../.. -I$(top_srcdir)/src -I$(top_srcdir)/src/libs $(WXBASE_CFLAGS) $(LIBPNG_CFLAGS) $(AMULEWEB_DEFS) $(BFD_FLAGS) -DEC_REMOTE -DWEBSERVERDIR="\"$(pkgdatadir)/webserver\""
amuleweb_CXXFLAGS = -I../.. -I$(top_srcdir)/src -I$(top_srcdir)/src/libs $(WXBASE_CXXFLAGS) $(LIBPNG_CXXFLAGS) $(AMULEWEB_DEFS) $(BFD_FLAGS) -DEC_REMOTE -DWEBSERVERDIR="\"$(pkgdatadir)/webserver\""
amuleweb_LDFLAGS = $(LIBPNG_LDFLAGS) -L../../libs/common -L../../libs/ec
amuleweb_LDADD = $(WXBASE_LIBS) $(ZLIB_LIBS) $(READLINE_LIBS) $(LIBPNG_LIBS) $(RESOLV_LIB) $(BFD_LIB) -lec -lmulecommon

noinst_HEADERS = \
		 WebInterface.h \
		 WebServer.h \
		 WebSocket.h
