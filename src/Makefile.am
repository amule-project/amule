if WEB
  OPT_WEB = webserver
  dist_html_DATA = aMule.tmpl
  htmldir = $(datadir)/amuleweb
else
if WEBGUI
  OPT_WEB = webserver
  dist_html_DATA = aMule.tmpl
  htmldir = $(datadir)/amuleweb
endif
endif

SUBDIRS = utils pixmaps $(OPT_WEB)

EXTRA_DIST = strings.en Makengfile aMule.xpm aMule.tmpl

bin_PROGRAMS = amule

if WEB
bin_PROGRAMS += amuleweb
endif

if WEBGUI
bin_PROGRAMS += amulewebDLG
endif

if GUI
bin_PROGRAMS += amulecmdDLG
endif

if COMPILE_CMD
bin_PROGRAMS += amulecmd   
endif

if ED2K
bin_PROGRAMS += ed2k
endif

DEFS = -DHAVE_CONFIG_H

BUILT_SOURCES =

LIBS = @LIBS@ 
amule_SOURCES = ListenSocket.cpp \
	ECSocket.cpp \
	EMSocket.cpp \
	AddFriend.cpp \
	AddFileThread.cpp \
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
	MuleGifCtrl.cpp \
	HTTPDownloadDlg.cpp \
	IPFilter.cpp \
	KnownFile.cpp \
	KnownFileList.cpp \
	listctrl.cpp \
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
	UDPSocket.cpp \
	UploadClient.cpp \
	UploadListCtrl.cpp \
	UploadQueue.cpp \
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

if VERBOSE_OUTPUT
amule_SOURCES += debugstuff.cpp
endif

amule_CFLAGS = $(TRAY_FLAGS) $(WX_CXXFLAGS) $(GTK_CFLAGS) $(GTK_DEFS) $(XRCFLAGS)
amule_CXXFLAGS = $(TRAY_FLAGS) $(WX_CXXFLAGS) $(GTK_CFLAGS) $(GTK_DEFS) $(XRCFLAGS) $(CURL_FLAGS)
amule_LDADD = $(WX_LIBS) $(GTK_LIBS) $(ZLIB_LIBS) $(CURL_LIBS) $(CRYPTOLIBS)

ed2k_SOURCES = ED2KLinkParser.cpp
ed2k_CFLAGS =

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

noinst_HEADERS = AddFileThread.h \
		 AddFriend.h \
		 amuleDlg.h \
		 amule.h \
		 amuleIPV4Address.h \
		 BarShader.h \
		 CArray.h \
		 CatDialog.h \
		 CFile.h \
		 ChatSelector.h \
		 ChatWnd.h \
		 ClientCredits.h \
		 ClientDetailDialog.h \
		 ClientList.h \
		 ClientUDPSocket.h \
		 CMD4Hash.h \
		 CMemFile.h \
		 ColorFrameCtrl.h \
		 color.h \
		 CommentDialog.h \
		 CommentDialogLst.h \
		 CTypedPtrList.h \
		 debugstuff.h \
		 DirectoryTreeCtrl.h \
		 DownloadListCtrl.h \
		 DownloadQueue.h \
		 ECSocket.h \
		 ED2KLink.h \
		 EditServerListDlg.h \
		 eggtrayicon.h \
		 EMSocket.h \
		 endianfix.h \
		 ExternalConn.h \
		 FileDetailDialog.h \
		 FileDetailListCtrl.h \
		 filefn.h \
		 Friend.h \
		 FriendListCtrl.h \
		 GetTickCount.h \
		 gsocket-2.5.h \
		 gsocket.h \
		 gtk2-funcs.h \
		 gtkplugxembed.h \
		 HTTPDownloadDlg.h \
		 inetdownload.h \
		 IPFilter.h \
		 KnownFile.h \
		 KnownFileList.h \
		 LastCommonRouteFinder.h \
		 listbase.h \
		 listctrl_gen.h \
		 listctrl.h \
		 ListenSocket.h \
		 MD5Sum.h \
		 MuleGifCtrl.h \
		 MuleListCtrl.h \
		 MuleNotebook.h \
		 muuli_wdr.h \
		 opcodes.h \
		 OScopeCtrl.h \
		 otherfunctions.h \
		 otherstructs.h \
		 packets.h \
		 PartFile.h \
		 Preferences.h \
		 PrefsUnifiedDlg.h \
		 Preview.h \
		 QueueListCtrl.h \
		 resource.h \
		 SafeFile.h \
		 SearchDlg.h \
		 SearchListCtrl.h \
		 SearchList.h \
		 server.h \
		 ServerListCtrl.h \
		 ServerList.h \
		 ServerSocket.h \
		 ServerWnd.h \
		 SharedFileList.h \
		 SharedFilesCtrl.h \
		 SharedFilesWnd.h \
		 sockets.h \
		 SplitterControl.h \
		 StatisticsDlg.h \
		 SysTray.h \
		 TextClient.h \
		 TransferWnd.h \
		 types.h \
		 UDPSocket.h \
		 updownclient.h \
		 UploadBandwidthThrottler.h \
		 UploadListCtrl.h \
		 UploadQueue.h \
		 WebInterface.h \
		 WebServer.h \
		 WebSocket.h \
		 xembed.h

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
