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

SUBDIRS = utils pixmaps kademlia $(OPT_WEB)

EXTRA_DIST = Makengfile aMule.tmpl

bin_PROGRAMS = 

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

if AMULE_GUI
bin_PROGRAMS += amulegui
endif

if AMULE_DAEMON
bin_PROGRAMS += amuled
endif

bin_PROGRAMS += amule

DEFS = -DHAVE_CONFIG_H

BUILT_SOURCES =

#LIBS = @LIBS@ 
amulegui_SOURCES = \
	#amule-gui.cpp \
	BarShader.cpp \
	CatDialog.cpp \
	CFile.cpp \
	ChatSelector.cpp \
	ChatWnd.cpp \
	ClientCredits.cpp \
	ClientDetailDialog.cpp \
	ColorFrameCtrl.cpp \
	CommentDialog.cpp \
	CommentDialogLst.cpp \
	DirectoryTreeCtrl.cpp \
	DownloadListCtrl.cpp \
	filemem.cpp \
	FileDetailDialog.cpp \
	FileDetailListCtrl.cpp \
	FriendListCtrl.cpp \
	listctrl.cpp \
	muuli_wdr.cpp \
	MuleListCtrl.cpp \
	MuleTextCtrl.cpp \
	MuleNotebook.cpp \
	MuleGifCtrl.cpp \
	otherfunctions.cpp \
	OScopeCtrl.cpp \
	packets.cpp \
	Preferences.cpp \
	PrefsUnifiedDlg.cpp \
	QueueListCtrl.cpp \
	SearchDlg.cpp \
	SearchList.cpp \
	SearchListCtrl.cpp \
	SharedFilesCtrl.cpp \
	SharedFilesWnd.cpp \
	ServerListCtrl.cpp \
	StatisticsDlg.cpp \
	TransferWnd.cpp \
	UploadListCtrl.cpp \
	amuleDlg.cpp 

amule_SOURCES = ListenSocket.cpp \
	EMSocket.cpp \
	AddFriend.cpp \
	AddFileThread.cpp \
	AICHSyncThread.cpp \
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
	ECSocket.cpp \
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
	HTTPDownload.cpp \
	IPFilter.cpp \
	KnownFile.cpp \
	KadDlg.cpp \
	KnownFileList.cpp \
	listctrl.cpp \
	MD5Sum.cpp \
	MuleListCtrl.cpp \
	MuleNotebook.cpp \
	MuleTextCtrl.cpp \
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
	SHA.cpp \
	SHAHashSet.cpp \
	sockets.cpp \
	StatisticsDlg.cpp \
	TransferWnd.cpp \
	UDPSocket.cpp \
	UploadClient.cpp \
	UploadListCtrl.cpp \
	UploadQueue.cpp \
	amule.cpp \
	amuleDlg.cpp \
	kademlia/utils/UInt128.cpp

amuled_SOURCES = ListenSocket.cpp \
	AICHSyncThread.cpp \
        ECSocket.cpp \
        EMSocket.cpp \
        AddFileThread.cpp \
        BaseClient.cpp \
        SHA.cpp \
        SHAHashSet.cpp \
        CFile.cpp \
        ClientCredits.cpp \
        ClientList.cpp \
        ClientUDPSocket.cpp \
        DownloadClient.cpp \
        DownloadQueue.cpp \
        ED2KLink.cpp \
        ExternalConn.cpp \
        filemem.cpp \
        filefn.cpp \
        Friend.cpp \
	HTTPDownload.cpp \
        IPFilter.cpp \
        KnownFile.cpp \
        KnownFileList.cpp \
        MD5Sum.cpp \
        otherfunctions.cpp \
        packets.cpp \
        PartFile.cpp \
        Preferences.cpp \
        SafeFile.cpp \
        SearchList.cpp \
        server.cpp \
        ServerList.cpp \
        ServerSocket.cpp \
        SharedFileList.cpp \
        sockets.cpp \
	timer.cpp \
        UDPSocket.cpp \
        UploadClient.cpp \
        UploadQueue.cpp \
        amuled.cpp

if ADD_GSOCKET
if ADD_24GSOCKET
amule_SOURCES += gsocket.c 
else
amule_SOURCES += gsocket-2.5.c
endif
endif

if HAVE_TRAY
amule_SOURCES += eggtrayicon.c SysTray.cpp 
amulegui_SOURCES += eggtrayicon.c SysTray.cpp 
endif

if HAVE_GTK
amule_SOURCES += gtk2-funcs.c gtkplugxembed.c 
amulegui_SOURCES += gtk2-funcs.c gtkplugxembed.c 
endif

if USE_EMBEDDED_CRYPTO
amule_SOURCES += CryptoPP.cpp
amuled_SOURCES += CryptoPP.cpp
endif

if ADD_THROTTLER
amule_SOURCES += UploadBandwidthThrottler.cpp LastCommonRouteFinder.cpp 
endif

amule_CFLAGS = $(UTF8_SYSTRAY_FLAGS) $(TRAY_FLAGS) $(WX_CXXFLAGS) $(GTK_CFLAGS) $(GTK_DEFS) $(XRCFLAGS)
amule_CXXFLAGS = $(UTF8_SYSTRAY_FLAGS) $(TRAY_FLAGS) $(WX_CXXFLAGS) $(GTK_CFLAGS) $(GTK_DEFS) $(XRCFLAGS) $(CURL_FLAGS)
amule_LDADD = $(WX_LIBS) $(GTK_LIBS) $(ZLIB_LIBS) $(CURL_LIBS) $(CRYPTOLIBS)

amulegui_CFLAGS = $(UTF8_SYSTRAY_FLAGS) $(TRAY_FLAGS) $(WX_CXXFLAGS) $(GTK_CFLAGS) $(GTK_DEFS) $(XRCFLAGS) -DCLIENT_GUI
amulegui_CXXFLAGS = $(UTF8_SYSTRAY_FLAGS) $(TRAY_FLAGS) $(WX_CXXFLAGS) $(GTK_CFLAGS) $(GTK_DEFS) $(XRCFLAGS) $(CURL_FLAGS) -DCLIENT_GUI
amulegui_LDADD = $(WX_LIBS) $(GTK_LIBS) $(ZLIB_LIBS) $(CURL_LIBS) $(CRYPTOLIBS)

amuled_CFLAGS = $(WX_CFLAGS) $(WX_CXXFLAGS) -DAMULE_DAEMON
amuled_CXXFLAGS = $(WX_CFLAGS) $(WX_CXXFLAGS) -DAMULE_DAEMON 
amuled_LDADD = $(WXBASE_LIBS) $(ZLIB_LIBS) $(CURL_LIBS) $(CRYPTOLIBS)

ed2k_SOURCES = ED2KLinkParser.cpp
ed2k_CFLAGS =

amulecmd_SOURCES = TextClient.cpp MD5Sum.cpp ECSocket.cpp ExternalConnector.cpp
amulecmd_CFLAGS = $(WXBASE_CFLAGS)
amulecmd_CXXFLAGS = $(WXBASE_CXXFLAGS)
amulecmd_LDADD = $(WXBASE_LIBS)


amulecmdDLG_SOURCES = TextClient.cpp MD5Sum.cpp ECSocket.cpp ExternalConnector.cpp
amulecmdDLG_CFLAGS = $(WX_CFLAGS) -DAMULECMDDLG
amulecmdDLG_CXXFLAGS = $(WX_CXXFLAGS) -DAMULECMDDLG 
amulecmdDLG_LDADD = $(WX_LIBS)

amuleweb_SOURCES =  WebServer.cpp WebInterface.cpp WebSocket.cpp \
	MD5Sum.cpp ECSocket.cpp ExternalConnector.cpp
amuleweb_CFLAGS = $(WXBASE_CFLAGS)
amuleweb_CXXFLAGS = $(WXBASE_CXXFLAGS)
amuleweb_LDADD = $(WXBASE_LIBS) $(ZLIB_LIBS)


amulewebDLG_SOURCES = WebServer.cpp WebInterface.cpp WebSocket.cpp \
	 MD5Sum.cpp ECSocket.cpp ExternalConnector.cpp
amulewebDLG_CFLAGS = $(WX_CFLAGS) -DAMULEWEBDLG
amulewebDLG_CXXFLAGS = $(WX_CXXFLAGS) -DAMULEWEBDLG 
amulewebDLG_LDADD = $(WX_LIBS) $(ZLIB_LIBS)

noinst_HEADERS = AddFileThread.h \
		 AddFriend.h \
		 AICHSyncThread.h \
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
		 CryptoPP.h \
		 CryptoPP_Inc.h \
		 CTypedPtrList.h \
		 DirectoryTreeCtrl.h \
		 DownloadListCtrl.h \
		 DownloadQueue.h \
		 ECSocket.h \
		 ED2KLink.h \
		 EditServerListDlg.h \
		 eggtrayicon.h \
		 EMSocket.h \
		 endianfix.h \
		 ExternalConnector.h \
		 ExternalConn.h \
		 FileDetailDialog.h \
		 FileDetailListCtrl.h \
		 filefn.h \
		 Friend.h \
		 FriendListCtrl.h \
		 GetTickCount.h \
		 gsocket-fix.h \
		 gsocket.h \
		 gtk2-funcs.h \
		 gtkplugxembed.h \
		 GuiEvents.h \
		 HTTPDownloadDlg.h \
		 inetdownload.h \
		 IPFilter.h \
		 KadDlg.h \
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
		 MuleTextCtrl.h \
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
		 SafeFile.h \
		 SearchDlg.h \
		 SearchListCtrl.h \
		 SearchList.h \
		 server.h \
		 ServerListCtrl.h \
		 ServerList.h \
		 ServerSocket.h \
		 ServerWnd.h \
		 SHA.h \
		 SHAHashSet.h \
		 SharedFileList.h \
		 SharedFilesCtrl.h \
		 SharedFilesWnd.h \
		 sockets.h \
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

