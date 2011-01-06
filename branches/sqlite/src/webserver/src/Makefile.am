bin_PROGRAMS = amuleweb

amuleweb_SOURCES =  \
	WebServer.cpp \
	WebInterface.cpp \
	WebSocket.cpp \
	$(top_srcdir)/src/ExternalConnector.cpp \
	$(top_srcdir)/src/OtherFunctions.cpp \
	$(top_srcdir)/src/RLE.cpp \
	$(top_srcdir)/src/NetworkFunctions.cpp \
	$(top_srcdir)/src/LoggerConsole.cpp \
	$(top_srcdir)/src/UPnPBase.cpp \
	php_parser.c \
	php_lexer.c \
	php_syntree.cpp \
	php_amule_lib.cpp \
	php_core_lib.cpp

php_parser.c: php_parser.y
	bison --debug -t -d -v -p php -o $@ $<

php_lexer.c: php_lexer.l
	$(LEX) -P php -o$@ $<

amuleweb_DEPENDENCIES = ../../libs/common/libmulecommon.a ../../libs/ec/cpp/libec.a
amuleweb_CPPFLAGS = -I../.. -I$(top_srcdir)/src -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/include \
	$(MULECPPFLAGS) $(AMULEWEB_DEFS) $(BFD_CPPFLAGS) $(WXBASE_CPPFLAGS) \
	-DWEBSERVERDIR="\"$(pkgdatadir)/webserver\"" \
	$(INCINTL) $(ZLIB_CPPFLAGS) $(LIBUPNP_CPPFLAGS)
amuleweb_CFLAGS = $(MULECFLAGS) $(WX_CFLAGS_ONLY) $(LIBPNG_CFLAGS) $(LIBUPNP_CFLAGS)
amuleweb_CXXFLAGS = $(MULECXXFLAGS) $(WX_CFLAGS_ONLY) $(WX_CXXFLAGS_ONLY) $(LIBPNG_CFLAGS) $(LIBUPNP_CFLAGS)

amuleweb_LDFLAGS = $(MULELDFLAGS) -L../../libs/common -L../../libs/ec/cpp $(LIBPNG_LDFLAGS) $(ZLIB_LDFLAGS) $(LIBUPNP_LDFLAGS)
amuleweb_LDADD = -lec -lmulecommon $(WXBASE_LIBS) $(BFD_LIBS) $(ZLIB_LIBS) \
	$(READLINE_LIBS) $(LIBPNG_LIBS) $(RESOLV_LIB) $(LIBINTL) $(LIBUPNP_LIBS)

noinst_HEADERS = \
		 WebInterface.h \
		 WebServer.h \
		 WebSocket.h \
		 php_core_lib.h \
		 php_parser.h \
		 php_syntree.h

# Resources

if NEED_RC

SUFFIXES = .rc

.rc.$(OBJEXT):
	$(RC) $(MULERCFLAGS) -I$(top_srcdir) $(RCFLAGS) -O COFF -i "$<" -o "$@"

nodist_amuleweb_SOURCES = amuleweb-version.rc

amuleweb-version.rc: $(top_builddir)/version.rc
	@sed -e 's/VER_FILEDESCRIPTION_STR/"aMule Web Server"/' \
	     -e 's/VER_INTERNALNAME_STR/"amuleweb"/' \
	     -e 's/VER_ORIGINALFILENAME_STR/"amuleweb$(EXEEXT)"/' "$<" > "$@"

CLEANFILES = *-version.rc

endif

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = \
	missing-template.html \
	php_lexer.l \
	php_parser.y

