bin_PROGRAMS = amuleweb

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

if ENABLE_UPNP
amuleweb_SOURCES +=  \
	$(top_srcdir)/src/UPnP.cpp \
	UPnPCompatibility.cpp
endif

php_parser.c: php_parser.y
	bison --debug -t -d -v -o $@ $<

php_lexer.c: php_lexer.l
	$(LEX) -o$@ $<

amuleweb_DEPENDENCIES = ../../libs/common/libmulecommon.a ../../libs/ec/cpp/libec.a
amuleweb_CPPFLAGS = -I../.. -I$(top_srcdir)/src -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/include \
	$(AMULEWEB_DEFS) $(BFD_FLAGS) \
	-DEC_REMOTE -DWEBSERVERDIR="\"$(pkgdatadir)/webserver\"" \
	-DECSOCKET_USE_EVENTS=0
amuleweb_CFLAGS = $(WXBASE_CFLAGS) $(LIBPNG_CFLAGS)
amuleweb_CXXFLAGS = $(WXBASE_CXXFLAGS) $(LIBPNG_CXXFLAGS)

if ENABLE_UPNP
amuleweb_CFLAGS += -DENABLE_UPNP
amuleweb_CXXFLAGS += -DENABLE_UPNP
endif

amuleweb_LDFLAGS = $(LIBPNG_LDFLAGS) -L../../libs/common -L../../libs/ec/cpp
amuleweb_LDADD = -lec -lmulecommon $(WXBASE_LIBS) $(ZLIB_LIBS) \
	$(READLINE_LIBS) $(LIBPNG_LIBS) $(RESOLV_LIB) $(BFD_LIB) $(REGEX_LIB)

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
	$(RC) $(RCFLAGS) -I$(top_srcdir) -D__WIN95__ -D__WIN32__ -D__GNUWIN32__ -O COFF -i "$<" -o "$@"

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
