bin_PROGRAMS =
if GUI
bin_PROGRAMS += amulecmdDLG
endif
if COMPILE_CMD
bin_PROGRAMS += amulecmd   
endif
if WEB
bin_PROGRAMS += amuleweb
htmldir = $(datadir)/amuleweb
dist_html_DATA = aMule.tmpl
htmlimgdir = $(htmldir)/webserver
dist_htmlimg_DATA = webserver/*.gif
endif
if WEBGUI
bin_PROGRAMS += amulewebDLG
htmldir = $(datadir)/amuleweb
dist_html_DATA = aMule.tmpl
htmlimgdir = $(htmldir)/webserver
dist_htmlimg_DATA = webserver/*.gif
endif
if CAS
bin_PROGRAMS += cas
endif
if WXCAS
bin_PROGRAMS += wxcas
endif
if ED2K
bin_PROGRAMS += ed2k
endif
bin_PROGRAMS += amule

DEFS = -DHAVE_CONFIG_H

BUILT_SOURCES =
EXTRA_DIST =  strings.en *.h pixmaps bitmaps Makengfile

LIBS = @LIBS@ 
amule_SOURCES = ListenSocket.cpp \
	ECSocket.cpp \
	EMSocket.cpp \
	AddFriend.cpp \
	AddFileThread.cpp \
	animate.cpp \
	BarShader.cpp \
	BaseClient.cpp \
	ChatSelector.cpp \
	ChatWnd.cpp \
	CatDialog.cpp \
	CFile.cpp \
	ClientCredits.cpp \
	ClientDetailDialog.cpp \
	ClientList.cpp \
	ClientUDPSocket.cpp \
	ColorFrameCtrl.cpp \
	CommentDialog.cpp \
	CommentDialogLst.cpp \
	DirectoryTreeCtrl.cpp \
	DownloadClient.cpp \
	DownloadListCtrl.cpp \
	DownloadQueue.cpp \
	ED2KLink.cpp \
	EditServerListDlg.cpp \
	ExternalConn.cpp \
	FileDetailDialog.cpp \
	FileDetailListCtrl.cpp \
	filemem.cpp \
	filefn.cpp \
	Friend.cpp \
	FriendListCtrl.cpp \
	HTTPDownloadDlg.cpp \
	ini2.cpp \
	IPFilter.cpp \
	CMD4Hash.cpp \
	KnownFile.cpp \
	KnownFileList.cpp \
	listctrl.cpp \
	mfc.cpp \
	MD5Sum.cpp \
	MuleListCtrl.cpp \
	MuleNotebook.cpp \
	muuli_wdr.cpp \
	OScopeCtrl.cpp \
	otherfunctions.cpp \
	packets.cpp \
	PartFile.cpp \
	Preferences.cpp \
	PrefsUnifiedDlg.cpp \
	Preview.cpp \
	QueueListCtrl.cpp \
	SafeFile.cpp \
	SearchDlg.cpp \
	SearchList.cpp \
	SearchListCtrl.cpp \
	server.cpp \
	ServerList.cpp \
	ServerListCtrl.cpp \
	ServerSocket.cpp \
	ServerWnd.cpp \
	SharedFileList.cpp \
	SharedFilesCtrl.cpp \
	SharedFilesWnd.cpp \
	sockets.cpp \
	SplitterControl.cpp \
	StatisticsDlg.cpp \
	TransferWnd.cpp \
	treebasc.cpp \
	treectlc.cpp \
	UDPSocket.cpp \
	UploadClient.cpp \
	UploadListCtrl.cpp \
	UploadQueue.cpp \
	XBMDraw.cpp \
	amule.cpp \
	amuleDlg.cpp 
	
if ADD_GSOCKET
if ADD_24GSOCKET
amule_SOURCES += gsocket.c
else
amule_SOURCES += gsocket-2.5.c
endif
endif

if HAVE_TRAY
amule_SOURCES += eggtrayicon.c SysTray.cpp 
endif

if HAVE_GTK
amule_SOURCES += gtk2-funcs.c gtkplugxembed.c 
endif

if ADD_THROTTLER
amule_SOURCES += UploadBandwidthThrottler.cpp LastCommonRouteFinder.cpp 
endif

amule_CFLAGS = $(TRAY_FLAGS) $(WX_CXXFLAGS) $(GTK_CFLAGS) $(GTK_DEFS) $(XRCFLAGS)
amule_CXXFLAGS = $(TRAY_FLAGS) $(WX_CXXFLAGS) $(GTK_CFLAGS) $(GTK_DEFS) $(XRCFLAGS) $(CURL_FLAGS)
amule_LDADD = $(WX_LIBS) $(GTK_LIBS) $(ZLIB_LIBS) $(CURL_LIBS) $(CRYPTOLIBS)

ed2k_SOURCES = ED2KLinkParser.cpp
ed2k_CFLAGS =

cas_SOURCES = utils/cas/cas.c \
	      utils/cas/functions.c \
              utils/cas/configfile.c \
              utils/cas/graphics.c

#cas_CXXFLAGS= ""
#cas_LIBS= "" 
cas_CFLAGS= $(GDLIB_CFLAGS) $(CAS_DEFS)
cas_LDADD= $(GDLIB_LDFLAGS) $(GDLIB_LIBS) 

wxcas_SOURCES = utils/wxCas/src/onlinesig.cpp \
		utils/wxCas/src/linuxmon.cpp \
		utils/wxCas/src/wxcas.cpp \
		utils/wxCas/src/wxcasprint.cpp \
		utils/wxCas/src/wxcascanvas.cpp \
		utils/wxCas/src/wxcasframe.cpp \
		utils/wxCas/src/wxcasprefs.cpp \
		utils/wxCas/src/wxcascte.cpp		
wxcas_CFLAGS = $(WX_CFLAGS)		
wxcas_CXXFLAGS = $(WX_CXXFLAGS)
wxcas_LDADD = $(WX_LIBS)

amulecmd_SOURCES = TextClient.cpp MD5Sum.cpp ECSocket.cpp
if HAVE_WXBASE
amulecmd_CFLAGS = $(WXBASE_CFLAGS)
amulecmd_CXXFLAGS = $(WXBASE_CXXFLAGS)
amulecmd_LDADD = $(WXBASE_LIBS)
else
amulecmd_CFLAGS = $(WX_CFLAGS)
amulecmd_CXXFLAGS = $(WX_CXXFLAGS)
amulecmd_LDADD = $(WX_LIBS)
endif

amulecmdDLG_SOURCES = TextClient.cpp MD5Sum.cpp ECSocket.cpp
amulecmdDLG_CFLAGS = $(WX_CFLAGS) -DAMULECMDDLG
amulecmdDLG_CXXFLAGS = $(WX_CXXFLAGS) -DAMULECMDDLG 
amulecmdDLG_LDADD = $(WX_LIBS)

amuleweb_SOURCES =  WebServer.cpp WebInterface.cpp WebSocket.cpp \
					MD5Sum.cpp ECSocket.cpp
if HAVE_WXBASE
amuleweb_CFLAGS = $(WXBASE_CFLAGS)
amuleweb_CXXFLAGS = $(WXBASE_CXXFLAGS)
amuleweb_LDADD = $(WXBASE_LIBS) $(ZLIB_LIBS)
else
amuleweb_CFLAGS = $(WX_CFLAGS)
amuleweb_CXXFLAGS = $(WX_CXXFLAGS)
amuleweb_LDADD = $(WX_LIBS) $(ZLIB_LIBS)
endif

amulewebDLG_SOURCES = WebServer.cpp WebInterface.cpp WebSocket.cpp \
					  MD5Sum.cpp ECSocket.cpp
amulewebDLG_CFLAGS = $(WX_CFLAGS) -DAMULEWEBDLG
amulewebDLG_CXXFLAGS = $(WX_CXXFLAGS) -DAMULEWEBDLG 
amulewebDLG_LDADD = $(WX_LIBS) $(ZLIB_LIBS)

MAINTAINERCLEANFILES = Makefile.in


# Special Exceptions
#
#listctrl.cpp:
#	$(CXX) $(INCLUDES) $(GTK_CFLAGS) $(DEFS) -c `$(WX_CONFIG_PATH) --cxxflags` listctrl.cpp
#
#MuleNotebook.cpp:
#	$(CXX) $(INCLUDES) $(GTK_CFLAGS) $(DEFS) -c `$(WX_CONFIG_PATH) --cxxflags` MuleNotebook.cpp
#
#SysTray.cpp:
#	$(CXX) $(INCLUDES) $(GTK_CFLAGS) $(DEFS) -I. -c `$(WX_CONFIG_PATH) --cxxflags` SysTray.cpp
#
#ED2KLinkParser.cpp:
#	$(CXX) ED2KLinkParser.cpp
#
# implementation

	
SUFFIXES=.cpp .c .cpp
	
.c.cpp :
	$(CC) -c $<

.cpp.cpp :
	$(CXX) -I. -c  $<

all: $(bin_PROGRAMS)


clean:
	rm -f *.o
